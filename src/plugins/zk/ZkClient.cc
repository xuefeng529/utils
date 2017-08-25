#include "plugins/zk/ZkClient.h"
#include "base/Logging.h"
#include "base/CurrentThread.h"

#include <errno.h>

namespace zk
{

void ZkClient::handleEvent(zhandle_t* zh, int type, int state, const char* path, void* watchCtx)
{
    assert(watchCtx != NULL);
    ZkClient* zkClient = static_cast<ZkClient*>(watchCtx);
    LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid()
        << ", recv event: type=" << zkClient->typeToString(type) << ", state="
        << zkClient->stateToString(state) << ", path=[" << path << "]";
    base::MutexLockGuard lock(zkClient->sessionStateLock_);
    if (zh != zkClient->handle_) 
    {
        LOG_WARN << "zhandle not match";
        return;
    }
   
    if (type == ZOO_SESSION_EVENT)
    {
        zkClient->handleSessionEvent(state);
        return;
    }

    if (NULL == path) 
    {
        LOG_WARN << "path is missing";
        return;
    }

    if (!zkClient->isValidPath(path))
    {
        LOG_WARN << "path is invalid: " << path;
        return;
    }

    if (type == ZOO_CREATED_EVENT)
    {
        zkClient->handleCreateEvent(path);
    }
    else if (type == ZOO_DELETED_EVENT)
    {
        zkClient->handleDeleteEvent(path);
    }
    else if (type == ZOO_CHANGED_EVENT)
    {
        zkClient->handleChangeEvent(path);
    }
    else if (type == ZOO_CHILD_EVENT)
    {
        zkClient->handleChildEvent(path);
    }
    else if (type == ZOO_NOTWATCHING_EVENT)
    {
        zkClient->handleWatchLostEvent(state, path);
    }
    else 
    {
        LOG_WARN << "unknown event type : " << type;
    }
}

ZkClient::ZkClient()
    : connectedLatch_(1),
      handle_(NULL),
      sessionState_(kDisconneted),
      sessionId_(-1)
{
}

ZkClient::~ZkClient()
{
    if (handle_ != NULL)
    {
        zookeeper_close(handle_);
    }
}

std::string ZkClient::typeToString(int type) const
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

std::string ZkClient::stateToString(int state) const
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

bool ZkClient::init(const std::string& servers, uint32_t sessionTimeout)
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
    assert(handle_ == NULL);
    servers_ = servers; 
    handle_ = zookeeper_init(servers_.c_str(), handleEvent, sessionTimeout, NULL, this, 0);
    if (NULL == handle_) 
    {
        LOG_ERROR << "zookeeper_init: " << zerror(errno);      
        return false;
    }

    connectedLatch_.wait();
  
    base::MutexLockGuard mutex(sessionStateLock_);
    if (sessionState_ != kConnected) 
    {
        zookeeper_close(handle_);
        handle_ = NULL;
        return false;
    }
   
    LOG_INFO << "zookeeper_init success";
    return true;
}

void ZkClient::handleSessionEvent(int state)
{
    if (state == ZOO_CONNECTED_STATE)
    {
        const clientid_t* clientId = zoo_client_id(handle_);
        if (clientId == NULL)
        {
            LOG_WARN << "zoo_client_id fail";
            return;
        }

        sessionId_ = clientId->client_id;
        sessionState_ = kConnected;        
        sessionTimeout_ = zoo_recv_timeout(handle_);
        connectedLatch_.countDown();
        LOG_INFO << "connected to zk server, session timeout: "
            << sessionTimeout_ << " ms";
    }
    else if (state == ZOO_CONNECTING_STATE || state == ZOO_ASSOCIATING_STATE)
    {
        if (kConnected == sessionState_)
        {
            LOG_INFO << "disconnect from zk server, enable timer: "
                << sessionTimeout_ << " ms";
        }
        sessionId_ = -1;
        sessionState_ = kConnecting;
        connectedLatch_.countDown();
    }
    else if (state == ZOO_AUTH_FAILED_STATE)
    {
        sessionId_ = -1;
        sessionState_ = kFailedAuth;
        connectedLatch_.countDown();
    }
    else if (state == ZOO_EXPIRED_SESSION_STATE)
    {
        sessionId_ = -1;
        sessionState_ = kTimeout;
        connectedLatch_.countDown();
        if (sessionTimeoutCallback_)
        {
            sessionTimeoutCallback_();
        }
    }
}

bool ZkClient::createPersistentNode(const std::string& path, const std::string& data)
{
    return create(path, data, 0, NULL);
}

bool ZkClient::createEphemeralNode(const std::string& path, const std::string& data)
{
    return create(path, data, ZOO_EPHEMERAL, NULL);
}

bool ZkClient::createEphemeralSequentialNode(const std::string& path, const std::string& data, std::string* retPath)
{
    return create(path, data, ZOO_EPHEMERAL | ZOO_SEQUENCE, retPath);
}

bool ZkClient::deleteNode(const std::string& path)
{
    assert(handle_ != NULL);
    if (!isValidPath(path)) 
    {
        LOG_ERROR << "invalid path: " << path;
        return false;
    }

    int ret = zoo_delete(handle_, path.c_str(), -1);
    if (ret != ZOK)
    {
        LOG_WARN << "zoo_delete fail : " << zerror(ret);
        return false;
        
    }
   
    LOG_INFO << "zoo_delete success";
    return true;
}

bool ZkClient::exist(const std::string& path, bool watch, bool* isExist) const
{
    assert(handle_ != NULL);
    if (!isValidPath(path))
    {
        LOG_ERROR << "invalid path: " << path;
        return false;
    }
   
    ErrorCode ret = existWrapper(path, watch, isExist);
    return (ret == kOk);
}

bool ZkClient::writeNode(const std::string& path, const std::string& data)
{
    assert(handle_ != NULL);
    if (!isValidPath(path))
    {
        LOG_ERROR << "invalid path: " << path;
        return false;
    }

    int ret = zoo_set(handle_, path.c_str(), data.c_str(), data.size(), -1);
    if (ret != ZOK)
    {
        LOG_WARN << "zoo_set fail : " << zerror(ret);
        return false;
    }

    LOG_INFO << "zoo_set success";
    return true;
}

bool ZkClient::readNode(const std::string& path, bool watch, std::string* data) const
{
    assert(handle_ != NULL);
    data->clear();
    if (!isValidPath(path))
    {
        LOG_ERROR << "invalid path: " << path;
        return false;
    }

    char* buffer = new char[kMaxNodeDataLen];
    int bufferLen = kMaxNodeDataLen;
    int ret = zoo_get(handle_, path.c_str(), watch, buffer, &bufferLen, NULL);
    if (ret != ZOK)
    {
        LOG_WARN << "zoo_get fail : " << zerror(ret);
        delete[] buffer;
        return false;
    }

    LOG_INFO << "zoo_get success";
    if (bufferLen < 0)
    {
        bufferLen = 0;
    }
    else if (bufferLen >= kMaxNodeDataLen)
    {
        bufferLen = kMaxNodeDataLen - 1;
    }
    buffer[bufferLen] = '\0';
    *data = buffer;
    delete[] buffer;
    return true;
}

bool ZkClient::getChildren(const std::string& path,
                           bool watch,
                           std::vector<std::string>* childList,
                           std::vector<std::string>* dataList) const
{
    assert(handle_ != NULL);
    if (!isValidPath(path))
    {
        LOG_ERROR << "invalid path: " << path;
        return false;
    }
    
    ErrorCode ret = getChildrenWrapper(path, watch, childList, dataList);
    return (ret == kOk);
}

int64_t ZkClient::sessionId() const
{
    base::MutexLockGuard lock(sessionStateLock_);
    return sessionId_;
}

bool ZkClient::create(const std::string& path,
                      const std::string& data,
                      int flag,
                      std::string* retPath)
{
  
    assert(handle_ != NULL);
    if (!isValidPath(path))
    {
        LOG_ERROR << "invalid path: " << path;
        return false;
    }

    int valueLen = data.size();
    if (valueLen == 0) 
    {
        valueLen = -1;
    }

    char* buf = NULL;
    int bufLen = 0;
    if (retPath != NULL)
    {
        bufLen = path.size() + 11;
        buf = new char[bufLen];
    }

    int ret = zoo_create(handle_, path.c_str(), data.c_str(), valueLen,
        &ZOO_OPEN_ACL_UNSAFE, flag, buf, bufLen);
    if (ret != ZOK)
    {
        LOG_ERROR << "zoo_create fail : " << zerror(ret);
        if (buf != NULL)
        {
            delete[] buf;
        }
        return false;
    }

    LOG_INFO << "zoo_create success";
    if (buf != NULL)
    {
        assert(retPath != NULL);
        *retPath = buf;
        delete[] buf;
    }
    return true;
}

bool ZkClient::isValidPath(const std::string& path) const
{
    if (path.empty() || path[0] != '/'
        || (path.size() > 1 && *path.rbegin() == '/'))
    {
        return false;
    }
    return true;
}

void ZkClient::handleCreateEvent(const std::string& path) 
{
    LOG_INFO << "handleCreateEvent: path=[" << path << "]";
    if (nodeCreatedCallback_)
    {
        nodeCreatedCallback_(path);
    }
   
    bool isExist;
    ErrorCode ret = existWrapper(path, true, &isExist);
    if (ret == kOk && !isExist)
    {          
        LOG_WARN << "node not exist" << path;
    }
}

void ZkClient::handleDeleteEvent(const std::string& path)
{
    LOG_INFO << "handleDeleteEvent: path=[" << path << "]";
    if (nodeDeletedCallback_)
    {
        nodeDeletedCallback_(path);
    }

    bool isExist;
    ErrorCode ret = existWrapper(path, true, &isExist);
    if (ret == kOk && isExist) 
    {      
        LOG_WARN << "node exist: " << path;
    }
}

void ZkClient::handleChangeEvent(const std::string& path)
{
    LOG_INFO << "handleChangeEvent: path=[" << path << "]";
    std::string data;
    ErrorCode ret = getWrapper(path, true, &data);
    if (ret == kOk)
    {
        if (nodeDataChangedCallback_)
        {
            nodeDataChangedCallback_(path, data);
        }      
    }
    else if (ret == kNotExist)
    {
        if (nodeDeletedCallback_)
        {
            nodeDeletedCallback_(path);
        }       
    }
    else
    {
        LOG_WARN << "unknow node status: " << path;
    }
}

void ZkClient::handleChildEvent(const std::string& path)
{
    LOG_INFO << "handleChildEvent: path=[" << path << "]";
    std::vector<std::string> childList;
    std::vector<std::string> dataList;
    ErrorCode ret = getChildrenWrapper(path, true, &childList, &dataList);
    if (ret == kOk)
    {
        if (childrenChangedCallback_)
        {
            childrenChangedCallback_(path, childList, dataList);
        }       
    }
    else if (ret == kNotExist)
    {     
        if (nodeDeletedCallback_)
        {
            nodeDeletedCallback_(path);
        }       
    }
    else
    {
        LOG_WARN << "unknow child event";
    }
}

void ZkClient::handleWatchLostEvent(int state, const std::string& path)
{
    LOG_INFO << "handleWatchLostEvent: type=" << stateToString(state) << ", path=[" << path << "]";
}

ErrorCode ZkClient::existWrapper(const std::string& path, bool watch, bool* isExist) const
{
    struct Stat stat;
    int ret = zoo_exists(handle_, path.c_str(), watch, &stat);
    if (ret == ZOK)
    {
        LOG_INFO << "zoo_exists success";
        *isExist = true;     
    }
    else if (ret == ZNONODE)
    {
        LOG_INFO << "zoo_exists success";
        *isExist = false;       
    }
    else
    {
        LOG_ERROR << "zoo_exists fail : " << zerror(ret);
    }

    switch (ret) 
    {
    case ZOK:
    case ZNONODE:
        return kOk;
    case ZNOAUTH:
        return kAuth;
    case ZBADARGUMENTS:
        return kArg;
    case ZINVALIDSTATE:
        return kSession;
    case ZMARSHALLINGERROR:
        return kSystem;
    default:
        return kUnknown;
    }
}

ErrorCode ZkClient::getWrapper(const std::string& path, bool watch, std::string* data) const
{
    char* buffer = new char[kMaxNodeDataLen];
    int bufferLen = kMaxNodeDataLen;
    int ret = zoo_get(handle_, path.c_str(), watch, buffer, &bufferLen, NULL);
    if (ret == ZOK)
    {
        LOG_INFO << "zoo_get success";
        if (bufferLen < 0)
        {
            bufferLen = 0;
        }
        else if (bufferLen >= kMaxNodeDataLen)
        {
            bufferLen = kMaxNodeDataLen - 1;
        }
        buffer[bufferLen] = '\0';
        *data = buffer;      
    }
    else 
    {
        LOG_WARN << "zoo_get fail : " << zerror(ret);
    }

    delete[] buffer;

    switch (ret)
    {
    case ZOK:
        return kOk;
    case ZNONODE:
        return kNotExist;
    case ZNOAUTH:
        return kAuth;
    case ZBADARGUMENTS:
        return kArg;
    case ZINVALIDSTATE:
        return kSession;
    case ZMARSHALLINGERROR:
        return kSystem;
    default:
        return kUnknown;
    }
}

ErrorCode ZkClient::getChildrenWrapper(const std::string& path,
                                       bool watch,
                                       std::vector<std::string>* childList,
                                       std::vector<std::string>* dataList) const
{
    struct String_vector strVec;
    allocate_String_vector(&strVec, 0);
    int ret = zoo_get_children(handle_, path.c_str(), watch, &strVec);
    if (ret == ZOK) 
    {
        LOG_INFO << "zoo_get_children success";
        childList->clear();
        dataList->clear();
        for (int i = 0; i < strVec.count; i++)
        {
            childList->push_back(strVec.data[i]);
            std::string childPath = path + '/' + strVec.data[i];
            std::string data;
            int ret2 = getWrapper(childPath, false, &data);
            if (ret2 != kOk) 
            {
                LOG_WARN << "read node fail : " << ret2;
                data = "";               
            }
            dataList->push_back(data);
        }   
    }
    else 
    {
        LOG_WARN << "zoo_get_children fail : " << zerror(ret);
    }

    deallocate_String_vector(&strVec);

    switch (ret)
    {
    case ZOK:
        return kOk;
    case ZNONODE:
        return kNotExist;
    case ZNOAUTH:
        return kAuth;
    case ZBADARGUMENTS:
        return kArg;
    case ZINVALIDSTATE:
        return kSession;
    case ZMARSHALLINGERROR:
        return kSystem;
    default:
        return kUnknown;
    }
}

} // namespace zk
