#include "plugins/zk/Client.h"
#include "base/Logging.h"
#include "base/CurrentThread.h"

#include "zookeeper/zookeeper.h"

#include <boost/bind.hpp>

#include <errno.h>

namespace plugins
{
namespace zk
{

static const char* stateToString(int state)
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

static const char* typeToString(int type)
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

static time_t getCurrentMs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

Client::Client()
    : started_(false),
      sessionLatch_(1),
      checkSessionThread_(boost::bind(&Client::checkSessionStateThreadFunc, this)),
      zkHandle_(NULL),
      sessionDisconnectMs_(getCurrentMs())
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
    LOG_ERROR << "session timeout.";
    sleep(3);
    exit(0);
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
    LOG_INFO << "session state: " << state << "," << stateToString(state) << " [handleSessionState]";
    base::MutexLockGuard lock(sessionStateLock_);
    sessionState_ = state;
    if (state == ZOO_CONNECTED_STATE)
    {
        sessionTimeout_ = zoo_recv_timeout(zhandle);
        sessionLatch_.countDown();
    }
    else if (state == ZOO_EXPIRED_SESSION_STATE)
    {
        sessionLatch_.countDown();
    }
    else
    {
        /// 连接异常，记录下异常开始时间，用于计算会话是否过期
        sessionDisconnectMs_ = getCurrentMs();
    }
}

bool Client::init(const std::string& host, uint32_t sessionTimeout, const SessionTimeoutCallback& cb)
{
    assert(!started_);
    sessionTimeout_ = sessionTimeout;
    if (!cb)
    {
        sessionTimeoutCb_ = boost::bind(&Client::defaultSessionTimeoutHandler, this);
    }
    
    sessionTimeoutCb_ = cb;
    zkHandle_ = zookeeper_init(host.c_str(), handleSessionWatcher, sessionTimeout_, NULL, this, 0);
    if (zkHandle_ == NULL)
    {
        LOG_ERROR << "zookeeper_init [" << host << "]";
        return false;
    }
    
    sessionLatch_.wait();
    base::MutexLockGuard lock(sessionStateLock_);
    if (sessionState_ == ZOO_EXPIRED_SESSION_STATE)
    {
        LOG_ERROR << "session timeout [" << host << "]";
        return false;
    }

    started_ = true;
    checkSessionThread_.start();
    return true;
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

void Client::handleDeleteCompletion(int rc, const void* ctx)
{
    assert(ctx != NULL);
    assert(rc == ZOK || rc == ZCONNECTIONLOSS || rc == ZOPERATIONTIMEOUT || rc == ZBADVERSION
           || rc == ZNOAUTH || rc == ZNONODE || rc == ZNOTEMPTY || rc == ZCLOSING);

    WatchContext* watchCtx = const_cast<WatchContext*>(static_cast<const WatchContext*>(ctx));
    LOG_DEBUG << "code: " << rc << "," << watchCtx->path_ << " [handleDeleteCompletion]";
    if (rc == ZOK)
    {
        watchCtx->deleteCb_(kOk, watchCtx->path_);
    }
    else if (rc == ZNONODE)
    {
        watchCtx->deleteCb_(kNotExist, watchCtx->path_);
    }
    else if (rc == ZNOTEMPTY)
    {
        watchCtx->deleteCb_(kNotEmpty, watchCtx->path_);
    }
    else
    {
        watchCtx->deleteCb_(kError, watchCtx->path_);
    }

    delete watchCtx;
}

bool Client::del(const std::string& path, const DeleteCallback& cb)
{
    WatchContext* ctx = new WatchContext(path, false);
    ctx->deleteCb_ = cb;
    int rc = zoo_adelete(zkHandle_, path.c_str(), -1, handleDeleteCompletion, ctx);
    return rc == ZOK ? true : false;
}

void Client::handleSetCompletion(int rc, const struct Stat* stat, const void* ctx)
{
    assert(ctx != NULL);
    assert(rc == ZOK || 
           rc == ZCONNECTIONLOSS || 
           rc == ZOPERATIONTIMEOUT || 
           rc == ZBADVERSION || 
           rc == ZNOAUTH || 
           rc == ZNONODE ||
           rc == ZCLOSING);

    WatchContext* watchCtx = const_cast<WatchContext*>(static_cast<const WatchContext*>(ctx));
    LOG_DEBUG << "code: " << rc << "," << watchCtx->path_ << " [handleSetCompletion]";
    if (rc == ZOK)
    {
        watchCtx->setCb_(kOk, watchCtx->path_, reinterpret_cast<const NodeStat*>(stat));
    }
    else if (rc == ZNONODE)
    {
        watchCtx->setCb_(kNotExist, watchCtx->path_, NULL);
    }
    else
    {
        watchCtx->setCb_(kError, watchCtx->path_, NULL);
    }

    delete watchCtx;
}

bool Client::set(const std::string& path, const std::string& data, const SetCallback& cb)
{
    assert(zkHandle_ != NULL);
    WatchContext* ctx = new WatchContext(path, false);
    ctx->setCb_ = cb;
    int rc = zoo_aset(zkHandle_, path.c_str(), data.data(), data.size(), -1, handleSetCompletion, ctx);
    return rc == ZOK ? true : false;
}

void Client::handleGetCompletion(int rc, const char* value, int len, const struct Stat* stat, const void* ctx)
{
    assert(ctx != NULL);
    assert(rc == ZOK || 
           rc == ZCONNECTIONLOSS || 
           rc == ZOPERATIONTIMEOUT || 
           rc == ZNOAUTH || 
           rc == ZNONODE || 
           rc == ZCLOSING);

    WatchContext* watchCtx = const_cast<WatchContext*>(static_cast<const WatchContext*>(ctx));
    LOG_DEBUG << "code: " << rc << "," << watchCtx->path_ << "," << std::string(value, len) << " [handleGetCompletion]";
    if (rc == ZOK)
    {
        watchCtx->getCb_(kOk, watchCtx->path_, std::string(value, len));
        if (!watchCtx->watch_)
        {
            delete watchCtx;
        }
        return;
    }

    if (rc == ZNONODE)
    {
        watchCtx->getCb_(kNotExist, watchCtx->path_, std::string(value, len));
    }
    else
    {
        watchCtx->getCb_(kError, watchCtx->path_, std::string(value, len));
    }
    
    delete watchCtx;
}

void Client::handleGetWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx)
{
    assert(ctx != NULL);
    assert(type == ZOO_DELETED_EVENT || 
           type == ZOO_CHANGED_EVENT || 
           type == ZOO_NOTWATCHING_EVENT || 
           type == ZOO_SESSION_EVENT);

    WatchContext* watchCtx = static_cast<WatchContext*>(ctx);
    LOG_DEBUG << "event type: " << type << "," << typeToString(type) << "," << path << " [handleGetWatcher]";
    if (type == ZOO_SESSION_EVENT) 
    { 
        LOG_WARN << "ZOO_SESSION_EVENT [handleGetWatcher]";
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
        }
        else if (type == ZOO_NOTWATCHING_EVENT)
        {
            LOG_WARN << "ZOO_NOTWATCHING_EVENT [handleGetWatcher]";
        }

        watchCtx->getCb_(kError, watchCtx->path_, std::string());
        delete watchCtx;
    }
}

bool Client::get(const std::string& path, const GetCallback& cb, bool watch)
{
    assert(zkHandle_ != NULL);
    watcher_fn watcher = watch ? handleGetWatcher : NULL;
    WatchContext* watchCtx = new WatchContext(path, watch);
    watchCtx->getCb_ = cb;
    int rc = zoo_awget(zkHandle_, path.c_str(), watcher, watchCtx, handleGetCompletion, watchCtx);
    return rc == ZOK ? true : false;
}

void Client::handleGetChildrenCompletion(int rc, const struct String_vector* strings, const void* ctx)
{
    assert(ctx != NULL);
    assert(rc == ZOK || 
           rc == ZCONNECTIONLOSS ||
           rc == ZOPERATIONTIMEOUT ||
           rc == ZNOAUTH || 
           rc == ZNONODE || 
           rc == ZCLOSING);

    WatchContext* watchCtx = const_cast<WatchContext*>(static_cast<const WatchContext*>(ctx));
    LOG_DEBUG << "code: " << rc << "," << watchCtx->path_ 
        << ",number of children: " << strings->count << " [handleGetChildrenCompletion]";
    if (rc == ZOK)
    {
        watchCtx->getChildrenCb_(kOk, watchCtx->path_,
                                 std::vector<std::string>(strings->data, strings->data + strings->count));
        if (!watchCtx->watch_)
        {
            delete watchCtx;
        }
        return;
    }
    if (rc == ZNONODE)
    {
        watchCtx->getChildrenCb_(kNotExist, watchCtx->path_, std::vector<std::string>());
    }
    else
    {
        watchCtx->getChildrenCb_(kError, watchCtx->path_, std::vector<std::string>());
    }
    
    delete watchCtx;
}

void Client::handleGetChildrenWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx)
{
    assert(ctx != NULL);
    assert(type == ZOO_DELETED_EVENT ||
           type == ZOO_CHILD_EVENT ||
           type == ZOO_NOTWATCHING_EVENT ||
           type == ZOO_SESSION_EVENT);

    WatchContext* watcherCtx = static_cast<WatchContext*>(ctx);
    LOG_DEBUG << "event type: " << type << "," << typeToString(type) << "," << path << " [handleGetChildrenWatcher]";
    if (type == ZOO_SESSION_EVENT)
    {
        LOG_WARN << "ZOO_SESSION_EVENT [handleGetChildrenWatcher]";
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
        }
        else if (type == ZOO_NOTWATCHING_EVENT)
        {
            LOG_WARN << "ZOO_NOTWATCHING_EVENT [handleGetChildrenWatcher]";
        }

        watcherCtx->getChildrenCb_(kError, watcherCtx->path_, std::vector<std::string>());
        delete watcherCtx;
    }
}

bool Client::getChildren(const std::string& path, const GetChildrenCallback cb, bool watch)
{
    assert(zkHandle_ != NULL);
    watcher_fn watcher = watch ? handleGetChildrenWatcher : NULL;
    WatchContext* watchCtx = new WatchContext(path, watch);
    watchCtx->getChildrenCb_ = cb;
    int rc = zoo_awget_children(zkHandle_, path.c_str(), watcher, watchCtx, handleGetChildrenCompletion, watchCtx);
    return rc == ZOK ? true : false;
}

void Client::handleExistCompletion(int rc, const struct Stat* stat, const void* ctx)
{
    assert(ctx != NULL);
    assert(rc == ZOK ||
           rc == ZCONNECTIONLOSS ||
           rc == ZOPERATIONTIMEOUT || 
           rc == ZNOAUTH || 
           rc == ZNONODE || 
           rc == ZCLOSING);

    WatchContext* watchCtx = const_cast<WatchContext*>(static_cast<const WatchContext*>(ctx));
    LOG_DEBUG << "code: " << rc << ",version: " << stat->aversion << "," << watchCtx->path_ << " [handleExistCompletion]";
    if (rc == ZOK || rc == ZNONODE)
    {
        watchCtx->existCb_(rc == ZOK ? kOk : kNotExist, watchCtx->path_, reinterpret_cast<const NodeStat*>(stat));
        if (!watchCtx->watch_)
        {
            delete watchCtx;
        }
        return;
    }

    watchCtx->existCb_(kError, watchCtx->path_, NULL);
    delete watchCtx;
}

void Client::handleExistWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx)
{
    assert(ctx != NULL);
    assert(type == ZOO_DELETED_EVENT || 
           type == ZOO_CREATED_EVENT || 
           type == ZOO_CHANGED_EVENT || 
           type == ZOO_NOTWATCHING_EVENT || 
           type == ZOO_SESSION_EVENT);

    WatchContext* watcherCtx = static_cast<WatchContext*>(ctx);
    LOG_DEBUG << "event type: " << type << "," << typeToString(type) << "," << path << " [handleExistWatcher]";
    if (type == ZOO_SESSION_EVENT)
    {
        LOG_WARN << "ZOO_SESSION_EVENT [handleExistWatcher]";
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
        int rc = zoo_awexists(zh, watcherCtx->path_.c_str(),
                              handleExistWatcher, watcherCtx, handleExistCompletion, watcherCtx);
        if (rc == ZOK)
        {
            return;
        }
        watcherCtx->existCb_(kError, watcherCtx->path_, NULL);
    }
    delete watcherCtx;
}

bool Client::exist(const std::string& path, const ExistCallback& cb, bool watch)
{
    assert(zkHandle_ != NULL);
    watcher_fn watcher = watch ? handleExistWatcher : NULL;
    WatchContext* watchCtx = new WatchContext(path, watch);
    watchCtx->existCb_ = cb;
    int rc = zoo_awexists(zkHandle_, path.c_str(), watcher, watchCtx, handleExistCompletion, watchCtx);
    return rc == ZOK ? true : false;
}

void Client::handleCreateCompletion(int rc, const char* value, const void* ctx)
{
    assert(ctx != NULL);
    assert(rc == ZOK || 
           rc == ZNODEEXISTS || 
           rc == ZCONNECTIONLOSS || 
           rc == ZOPERATIONTIMEOUT || 
           rc == ZNOAUTH || 
           rc == ZNONODE || 
           rc == ZNOCHILDRENFOREPHEMERALS || 
           rc == ZCLOSING);

    WatchContext* watchCtx = const_cast<WatchContext*>(static_cast<const WatchContext*>(ctx));
    LOG_DEBUG << "code: " << rc << "," << watchCtx->path_ << "," << value << " [handleCreateCompletion]";
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

bool Client::create(const std::string& path,
                    const std::string& data,
                    int flag,
                    const CreateCallback& cb)
{
    assert(zkHandle_ != NULL);
    WatchContext* watchCtx = new WatchContext(path, false);
    watchCtx->createCb_ = cb;
    int rc = zoo_acreate(zkHandle_, path.c_str(), data.data(), data.size(),
                         &ZOO_OPEN_ACL_UNSAFE, flag, handleCreateCompletion, watchCtx);
    return rc == ZOK ? true : false;
}

} // namespace zk
} // namespace plugins
