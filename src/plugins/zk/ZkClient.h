#ifndef PLUGIN_ZK_ZKCLIENT_H
#define PLUGIN_ZK_ZKCLIENT_H

#include "base/CountDownLatch.h"
#include "base/Mutex.h"

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <string>
#include <vector>

#include "zookeeper/zookeeper.h"

namespace zk 
{

enum ErrorCode
{
    kOk = 0,
    kArg,
    kSession,
    kSystem,
    kInited,
    kNotInit,
    kExist,
    kNotExist,
    kNoParent,
    kEntityParent,
    kAuth,
    kHasChild,
    kLockTimeout,
    kLockExist,
    kLockNotExist,
    kLockCanceled,
    kLockAcquired,
    kLockNotAcquired,
    kUnknown
};

enum SessionState 
{
    kDisconneted,
    kConnecting,
    kConnected,
    kFailedAuth,
    kTimeout
};

typedef boost::function <void ()> SessionTimeoutCallback;
typedef boost::function <void (const std::string&)> NodeCreatedCallback;
typedef boost::function <void (const std::string&)> NodeDeletedCallback;
typedef boost::function <void (const std::string&,
                              const std::vector<std::string>&,
                              const std::vector<std::string>&)> ChildrenChangedCallback;
typedef boost::function <void (const std::string&, const std::string&)> NodeDataChangedCallback;

class ZkClient : boost::noncopyable
{
public:
    ZkClient();
    ~ZkClient();

    bool init(const std::string& servers, uint32_t sessionTimeout);
    bool createPersistentNode(const std::string& path, const std::string& data);
    bool createEphemeralNode(const std::string& path, const std::string& data);
    bool createEphemeralSequentialNode(const std::string& path, const std::string& data, std::string* retPath);
    bool deleteNode(const std::string& path);
    bool exist(const std::string& path, bool watch, bool* isExist) const;
    bool writeNode(const std::string& path, const std::string& data);
    bool readNode(const std::string& path, bool watch, std::string* data) const;
    bool getChildren(const std::string& path, 
                     bool watch, 
                     std::vector<std::string>* childList,
                     std::vector<std::string>* dataList) const;

    int64_t sessionId() const;

private:
    static void handleEvent(zhandle_t* zh, int type, int state, const char* path, void* watchCtx);  

    static const int32_t kMaxNodeDataLen = 10240;

    bool isValidPath(const std::string& path) const;
    std::string typeToString(int type) const;
    std::string stateToString(int state) const;
    bool create(const std::string& path, 
                const std::string& data,
                int flag,
                std::string* retPath);

    void handleSessionEvent(int state);
    void handleCreateEvent(const std::string& path);
    void handleDeleteEvent(const std::string& path);
    void handleChangeEvent(const std::string& path);
    void handleChildEvent(const std::string& path);
    void handleWatchLostEvent(int state, const std::string& path);

    ErrorCode existWrapper(const std::string& path, bool watch, bool* isExist) const;
    ErrorCode getWrapper(const std::string& path, bool watch, std::string* data) const;
    //ErrorCode existWrapperForLock(const std::string& path, bool* isExist);
    ErrorCode getChildrenWrapper(const std::string& path,
                                 bool watch,
                                 std::vector<std::string>* childList,
                                 std::vector<std::string>* dataList) const;
    
    base::CountDownLatch connectedLatch_;
    zhandle_t* handle_;
    std::string servers_;
    mutable base::MutexLock sessionStateLock_;
    SessionState sessionState_;
    int64_t sessionId_;
    uint32_t sessionTimeout_;
    SessionTimeoutCallback sessionTimeoutCallback_;
    NodeCreatedCallback nodeCreatedCallback_;
    NodeDeletedCallback nodeDeletedCallback_;
    ChildrenChangedCallback childrenChangedCallback_;
    NodeDataChangedCallback nodeDataChangedCallback_;
};

} // namespace zk

#endif // PLUGIN_ZK_ZKCLIENT_H
