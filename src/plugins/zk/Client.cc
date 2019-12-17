#include "plugins/zk/Client.h"
#include "base/CurrentThread.h"
#include "base/Logging.h"

#include <zookeeper/zookeeper.h>

#include <boost/bind.hpp>

#include <errno.h>

namespace plugins
{
namespace zk
{

namespace
{

const char* stateToString(int state)
{
    if (state == ZOO_EXPIRED_SESSION_STATE)
    {
        return "ZOO_EXPIRED_SESSION_STATE";
    }
    else if (state == ZOO_AUTH_FAILED_STATE)
    {
        return "ZOO_AUTH_FAILED_STATE";
    }
    else if (state == ZOO_CONNECTING_STATE)
    {
        return "ZOO_CONNECTING_STATE";
    }
    else if (state == ZOO_ASSOCIATING_STATE)
    {
        return "ZOO_ASSOCIATING_STATE";
    }
    else if (state == ZOO_CONNECTED_STATE)
    {
        return "ZOO_CONNECTED_STATE";
    }
    else
    {
        return "ZOO_UNKNOWN_STATE";
    }
}

const char* typeToString(int type)
{
    if (type == ZOO_CREATED_EVENT)
    {
        return "ZOO_CREATED_EVENT";
    }
    else if (type == ZOO_DELETED_EVENT)
    {
        return "ZOO_DELETED_EVENT";
    }
    else if (type == ZOO_CHANGED_EVENT)
    {
        return "ZOO_CHANGED_EVENT";
    }
    else if (type == ZOO_CHILD_EVENT)
    {
        return "ZOO_CHILD_EVENT";
    }
    else if (type == ZOO_SESSION_EVENT)
    {
        return "ZOO_SESSION_EVENT";
    }
    else if (type == ZOO_NOTWATCHING_EVENT)
    {
        return "ZOO_NOTWATCHING_EVENT";
    }
    else
    {
        return "ZOO_UNKNOWN_EVENT";
    }
}

time_t getCurrentMs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

}

Client::Client()
    : started_(false),
      checkSessionThread_(boost::bind(&Client::checkSessionStateThreadFunc, this)),
      zkHandle_(NULL),
      sessionDisconnectMs_(getCurrentMs()),
	  sessionTimeoutCb_(boost::bind(&Client::defaultSessionTimeoutHandler, this))
{
}

Client::~Client()
{
    started_ = false;
    checkSessionThread_.join();
    if (zkHandle_ != NULL)
    {
        zookeeper_close(zkHandle_);
    }
}

bool Client::init(const std::string& hosts, uint32_t sessionTimeout, const SessionTimeoutCallback& cb, ZkLogLevel logLevel)
{
    assert(!started_);
	hosts_ = hosts;
    sessionTimeout_ = sessionTimeout;
    if (cb)
    {
		sessionTimeoutCb_ = cb;
    }
    
	zoo_set_debug_level(static_cast<ZooLogLevel>(logLevel));
	if (!reconnect())
	{
		return false;
	}
	
    started_ = true;
    checkSessionThread_.start();
    return true;
}

bool Client::reconnect()
{
	if (zkHandle_ != NULL)
	{
		zookeeper_close(zkHandle_);
	}

	sessionLatch_.reset(new base::CountDownLatch(1));
	zkHandle_ = zookeeper_init(hosts_.c_str(), handleSessionWatcher, sessionTimeout_, NULL, this, 0);
	if (zkHandle_ == NULL)
	{
		LOG_ERROR << "@zk: connecting zk server failed, hosts: " << hosts_ << ", timeout: " << sessionTimeout_;
		return false;
	}

	sessionLatch_->wait();
	base::MutexLockGuard lock(sessionStateLock_);
	if (sessionState_ != ZOO_CONNECTED_STATE)
	{
		LOG_ERROR << "@zk: connecting zk server failed, hosts: " << hosts_ << ", timeout: " << sessionTimeout_
			<< ", state: " << sessionState_;
		return false;
	}

	LOG_INFO << "@zk: connecting zk server successful, hosts: " << hosts_;
	return true;
}

void Client::handleSessionWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx)
{
	assert(ctx != NULL);
	assert(type == ZOO_SESSION_EVENT);
	Client* client = static_cast<Client*>(ctx);
	client->handleSessionState(zh, state);
}

void Client::handleSessionState(zhandle_t* zhandle, int state)
{
	LOG_INFO << "@zk: handle session state: " << state << ", state string: " << stateToString(state);
	base::MutexLockGuard lock(sessionStateLock_);
	sessionState_ = state;
	if (state == ZOO_CONNECTED_STATE)
	{
		sessionTimeout_ = zoo_recv_timeout(zhandle);
		LOG_INFO << "@zk: session established, timeout: " << sessionTimeout_;
		if (sessionLatch_->getCount() > 0)
		{
			sessionLatch_->countDown();
		}
		
	}
	else if (state == ZOO_EXPIRED_SESSION_STATE)
	{
		if (sessionLatch_->getCount() > 0)
		{
			sessionLatch_->countDown();
		}
	}
	else
	{
		/// 连接异常，记录下异常开始时间，用于计算会话是否过期
		sessionDisconnectMs_ = getCurrentMs();
	}
}

void Client::checkSessionStateThreadFunc()
{
	while (started_)
	{
		bool sessionTimeout = false;

		{
			base::MutexLockGuard lock(sessionStateLock_);
			if (sessionState_ == ZOO_EXPIRED_SESSION_STATE)
			{
				sessionTimeout = true;
			}
			else if (sessionState_ != ZOO_CONNECTED_STATE)
			{
				if (getCurrentMs() - sessionDisconnectMs_ > sessionTimeout_)
				{
					sessionTimeout = true;
				}
			}
		}

		if (sessionTimeout)
		{
			sessionTimeoutCb_();
		}

		usleep(1000);
	}
}

void Client::defaultSessionTimeoutHandler()
{
	LOG_ERROR << "@zk: the session timeout, hosts: " << hosts_;
	sleep(3);
	exit(EXIT_FAILURE);
}

bool Client::createPersistent(const std::string& path, const std::string& data, const CreateCallback& cb)
{
    return create(path, data, 0, cb);
}

bool Client::createPersistentSequential(const std::string& path, const std::string& data, const CreateCallback& cb)
{
    return create(path, data, ZOO_SEQUENCE, cb);
}

bool Client::createEphemeral(const std::string& path, const std::string& data, const CreateCallback& cb)
{
    return create(path, data, ZOO_EPHEMERAL, cb);
}

bool Client::createEphemeralSequential(const std::string& path, const std::string& data, const CreateCallback& cb)
{
    return create(path, data, ZOO_EPHEMERAL | ZOO_SEQUENCE, cb);
}

bool Client::create(const std::string& path,
					const std::string& data,
					int flag,
					const CreateCallback& cb)
{
	assert(zkHandle_ != NULL);
	Context* ctx = new Context(path, false);
	ctx->createCb_ = cb;
	int rc = zoo_acreate(zkHandle_, path.c_str(), data.data(), data.size(),
		&ZOO_OPEN_ACL_UNSAFE, flag, handleCreateCompletion, ctx);
	if (rc != ZOK)
	{
		LOG_ERROR << "@zk: zoo_acreate failed: error: " << zerror(rc);
		return false;
	}
	
	return true;
}

void Client::handleCreateCompletion(int rc, const char* value, const void* ctx)
{
    assert(ctx != NULL);
	Context* watchCtx = const_cast<Context*>(static_cast<const Context*>(ctx));
	LOG_INFO << "@zk: handle create completion, code: " << rc << ", path: " 
		<< watchCtx->path_ << ", value: " << value;
    assert(rc == ZOK || 
           rc == ZNODEEXISTS || 
           rc == ZCONNECTIONLOSS || 
           rc == ZOPERATIONTIMEOUT || 
           rc == ZNOAUTH || 
           rc == ZNONODE || 
           rc == ZNOCHILDRENFOREPHEMERALS || 
           rc == ZCLOSING);

    if (rc == ZOK)
    {
        watchCtx->createCb_(kOk, std::string(value));
    }
    else if (rc == ZNONODE)
    {
        watchCtx->createCb_(kNotExist, std::string());
    }
    else if (rc == ZNODEEXISTS)
    {
        watchCtx->createCb_(kExisted, std::string());
    }
    else
    {
        watchCtx->createCb_(kError, std::string());
    }

    delete watchCtx;
}

bool Client::del(const std::string& path, const DeleteCallback& cb)
{
	Context* ctx = new Context(path, false);
    ctx->deleteCb_ = cb;
	int rc = zoo_adelete(zkHandle_, path.c_str(), -1, handleDeleteCompletion, ctx);
	if (rc != ZOK)
	{
		LOG_ERROR << "@zk: zoo_adelete failed: error: " << zerror(rc);
		return false;
	}

	return true;
}

void Client::handleDeleteCompletion(int rc, const void* ctx)
{
	assert(ctx != NULL);
	Context* data = const_cast<Context*>(static_cast<const Context*>(ctx));
	LOG_INFO << "@zk: handle delete completion, code: " << rc << ", path: " << data->path_;
	assert(rc == ZOK || 
		   rc == ZCONNECTIONLOSS || 
		   rc == ZOPERATIONTIMEOUT ||
		   rc == ZBADVERSION ||
		   rc == ZNOAUTH ||
		   rc == ZNONODE ||
		   rc == ZNOTEMPTY ||
		   rc == ZCLOSING);

	if (rc == ZOK)
	{
		data->deleteCb_(kOk, data->path_);
	}
	else if (rc == ZNONODE)
	{
		data->deleteCb_(kNotExist, data->path_);
	}
	else if (rc == ZNOTEMPTY)
	{
		data->deleteCb_(kNotEmpty, data->path_);
	}
	else
	{
		data->deleteCb_(kError, data->path_);
	}

	delete data;
}

bool Client::set(const std::string& path, const std::string& data, const SetCallback& cb)
{
	assert(zkHandle_ != NULL);
	Context* ctx = new Context(path, false);
	ctx->setCb_ = cb;
	int rc = zoo_aset(zkHandle_, path.c_str(), data.data(), data.size(), -1, handleSetCompletion, ctx);
	if (rc != ZOK)
	{
		LOG_ERROR << "@zk: zoo_aset failed: error: " << zerror(rc);
		return false;
	}

	return true;
}

void Client::handleSetCompletion(int rc, const struct Stat* stat, const void* ctx)
{
    assert(ctx != NULL);
	Context* data = const_cast<Context*>(static_cast<const Context*>(ctx));
	LOG_INFO << "@zk: handle set completion, code: " << rc << ", path: " << data->path_ 
		<< ", value version: " << stat->version;
    assert(rc == ZOK || 
           rc == ZCONNECTIONLOSS || 
           rc == ZOPERATIONTIMEOUT || 
           rc == ZBADVERSION || 
           rc == ZNOAUTH || 
           rc == ZNONODE ||
           rc == ZCLOSING);

    if (rc == ZOK)
    {
		data->setCb_(kOk, data->path_, reinterpret_cast<const NodeStat*>(stat));
    }
    else if (rc == ZNONODE)
    {
		data->setCb_(kNotExist, data->path_, NULL);
    }
    else
    {
		data->setCb_(kError, data->path_, NULL);
    }

	delete data;
}

bool Client::get(const std::string& path, const GetCallback& cb, bool watch)
{
	assert(zkHandle_ != NULL);
	watcher_fn watcher = watch ? handleGetWatcher : NULL;
	Context* ctx = new Context(path, watch);
	ctx->getCb_ = cb;
	int rc = zoo_awget(zkHandle_, path.c_str(), watcher, ctx, handleGetCompletion, ctx);
	if (rc != ZOK)
	{
		LOG_ERROR << "@zk: zoo_awget failed: error: " << zerror(rc);
		return false;
	}

	return true;
}

void Client::handleGetWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx)
{
	assert(ctx != NULL);
	Context* watchCtx = static_cast<Context*>(ctx);
	LOG_INFO << "@zk: handle get watcher, type: " << type << ", type string: " << typeToString(type)
		<< ", state: " << state << ", state string: " << stateToString(state) << ", path: " << path;
	assert(type == ZOO_DELETED_EVENT ||
		   type == ZOO_CHANGED_EVENT ||
		   type == ZOO_NOTWATCHING_EVENT ||
		   type == ZOO_SESSION_EVENT);

	if (type == ZOO_SESSION_EVENT)
	{
		return;
	}

	if (type == ZOO_DELETED_EVENT)
	{
		watchCtx->getCb_(kDeleted, watchCtx->path_, std::string());
		delete watchCtx;
	}
	else
	{
		if (type == ZOO_CHANGED_EVENT)
		{
			int rc = zoo_awget(zh, watchCtx->path_.c_str(), handleGetWatcher, watchCtx, handleGetCompletion, watchCtx);
			if (rc == ZOK)
			{
				return;
			}

			LOG_ERROR << "@zk: zoo_awget failed: error: " << zerror(rc);	
		}
		else if (type == ZOO_NOTWATCHING_EVENT)
		{
		}

		watchCtx->getCb_(kError, watchCtx->path_, std::string());
		delete watchCtx;
	}
}

void Client::handleGetCompletion(int rc, const char* value, int len, const struct Stat* stat, const void* ctx)
{
    assert(ctx != NULL);
	Context* data = const_cast<Context*>(static_cast<const Context*>(ctx));
	LOG_INFO << "@zk: handle get completion, code: " << rc << ", path: " << data->path_ 
		<< ", value: " << std::string(value, len) << ", value version: " << stat->version;
    assert(rc == ZOK || 
           rc == ZCONNECTIONLOSS || 
           rc == ZOPERATIONTIMEOUT || 
           rc == ZNOAUTH || 
           rc == ZNONODE || 
           rc == ZCLOSING);

    if (rc == ZOK)
    {
		data->getCb_(kOk, data->path_, std::string(value, len));
		if (!data->watch_)
        {
			delete data;
        }
        return;
    }

    if (rc == ZNONODE)
    {
		data->getCb_(kNotExist, data->path_, std::string(value, len));
    }
    else
    {
		data->getCb_(kError, data->path_, std::string(value, len));
    }
    
	delete data;
}

bool Client::getChildren(const std::string& path, const GetChildrenCallback cb, bool watch)
{
	assert(zkHandle_ != NULL);
	watcher_fn watcher = watch ? handleGetChildrenWatcher : NULL;
	Context* ctx = new Context(path, watch);
	ctx->getChildrenCb_ = cb;
	int rc = zoo_awget_children(zkHandle_, path.c_str(), watcher, ctx, handleGetChildrenCompletion, ctx);
	if (rc != ZOK)
	{
		LOG_ERROR << "@zk: zoo_awget_children failed: error: " << zerror(rc);
		return false;
	}

	return true;
}

void Client::handleGetChildrenWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx)
{
	assert(ctx != NULL);
	Context* watcherCtx = static_cast<Context*>(ctx);
	LOG_INFO << "@zk: handle get children watcher, type: " << type << ", type string: " << typeToString(type)
		<< ", state: " << state << ", state string: " << stateToString(state) << ", path: " << path;
	assert(type == ZOO_DELETED_EVENT ||
		   type == ZOO_CHILD_EVENT ||
		   type == ZOO_NOTWATCHING_EVENT ||
		   type == ZOO_SESSION_EVENT);

	if (type == ZOO_SESSION_EVENT)
	{
		return;
	}

	if (type == ZOO_DELETED_EVENT)
	{
		watcherCtx->getChildrenCb_(kDeleted, watcherCtx->path_, std::vector<std::string>());
		delete watcherCtx;
	}
	else
	{
		if (type == ZOO_CHILD_EVENT)
		{
			int rc = zoo_awget_children(zh, watcherCtx->path_.c_str(), handleGetChildrenWatcher, watcherCtx,
				handleGetChildrenCompletion, watcherCtx);
			if (rc == ZOK)
			{
				return;
			}

			LOG_ERROR << "@zk: zoo_awget_children failed: error: " << zerror(rc);
		}
		else if (type == ZOO_NOTWATCHING_EVENT)
		{
		}

		watcherCtx->getChildrenCb_(kError, watcherCtx->path_, std::vector<std::string>());
		delete watcherCtx;
	}
}

void Client::handleGetChildrenCompletion(int rc, const struct String_vector* strings, const void* ctx)
{
    assert(ctx != NULL);
	Context* data = const_cast<Context*>(static_cast<const Context*>(ctx));
	LOG_INFO << "@zk: handle get children completion, code: " << rc << ", path: " << data->path_
		<< ", number of children: " << strings->count;
    assert(rc == ZOK || 
           rc == ZCONNECTIONLOSS ||
           rc == ZOPERATIONTIMEOUT ||
           rc == ZNOAUTH || 
           rc == ZNONODE || 
           rc == ZCLOSING);

    if (rc == ZOK)
    {
        data->getChildrenCb_(kOk, data->path_, std::vector<std::string>(
			strings->data, strings->data + strings->count));
        if (!data->watch_)
        {
            delete data;
        }
        return;
    }
    if (rc == ZNONODE)
    {
		data->getChildrenCb_(kNotExist, data->path_, std::vector<std::string>());
    }
    else
    {
		data->getChildrenCb_(kError, data->path_, std::vector<std::string>());
    }
    
    delete data;
}

bool Client::exist(const std::string& path, const ExistCallback& cb, bool watch)
{
	assert(zkHandle_ != NULL);
	watcher_fn watcher = watch ? handleExistWatcher : NULL;
	Context* ctx = new Context(path, watch);
	ctx->existCb_ = cb;
	int rc = zoo_awexists(zkHandle_, path.c_str(), watcher, ctx, handleExistCompletion, ctx);
	if (rc != ZOK)
	{
		LOG_ERROR << "@zk: zoo_awexists failed: error: " << zerror(rc);
		return false;
	}

	return true;
}

void Client::handleExistWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx)
{
	assert(ctx != NULL);
	Context* watcherCtx = static_cast<Context*>(ctx);
	LOG_INFO << "@zk: handle exist watcher, type: " << type << ", type string: " << typeToString(type)
		<< ", state: " << state << ", state string: " << stateToString(state) << ", path: " << path;
	assert(type == ZOO_DELETED_EVENT ||
		   type == ZOO_CREATED_EVENT ||
		   type == ZOO_CHANGED_EVENT ||
		   type == ZOO_NOTWATCHING_EVENT ||
		   type == ZOO_SESSION_EVENT);

	if (type == ZOO_SESSION_EVENT)
	{
		return;
	}

	if (type == ZOO_NOTWATCHING_EVENT)
	{
		watcherCtx->existCb_(kError, watcherCtx->path_, NULL);
	}
	else if (type == ZOO_DELETED_EVENT)
	{
		watcherCtx->existCb_(kDeleted, watcherCtx->path_, NULL);
	}
	else if (type == ZOO_CREATED_EVENT || type == ZOO_CHANGED_EVENT)
	{
		int rc = zoo_awexists(zh, watcherCtx->path_.c_str(), handleExistWatcher, 
			watcherCtx, handleExistCompletion, watcherCtx);
		if (rc == ZOK)
		{
			return;
		}

		LOG_ERROR << "@zk: zoo_awexists failed: error: " << zerror(rc);
		watcherCtx->existCb_(kError, watcherCtx->path_, NULL);
	}

	delete watcherCtx;
}

void Client::handleExistCompletion(int rc, const struct Stat* stat, const void* ctx)
{
    assert(ctx != NULL);
	Context* data = const_cast<Context*>(static_cast<const Context*>(ctx));
	LOG_INFO << "@zk: handle exist completion, code: " << rc << ", path: " << data->path_ 
		<<  ", value version: " << stat->aversion;
    assert(rc == ZOK ||
           rc == ZCONNECTIONLOSS ||
           rc == ZOPERATIONTIMEOUT || 
           rc == ZNOAUTH || 
           rc == ZNONODE || 
           rc == ZCLOSING);
   
    if (rc == ZOK || rc == ZNONODE)
    {
		data->existCb_(rc == ZOK ? kOk : kNotExist, data->path_, reinterpret_cast<const NodeStat*>(stat));
		if (!data->watch_)
        {
			delete data;
        }
        return;
    }

	data->existCb_(kError, data->path_, NULL);
	delete data;
}

} // namespace zk
} // namespace plugins
