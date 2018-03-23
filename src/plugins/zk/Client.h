#ifndef PLUGINS_ZK_CLIENT_H
#define PLUGINS_ZK_CLIENT_H

#include "base/Mutex.h"
#include "base/Thread.h"
#include "base/CountDownLatch.h"

#include <vector>

struct Stat;
struct String_vector;
typedef struct _zhandle zhandle_t;

namespace plugins
{
namespace zk 
{

enum ErrorCode
{
    kOk = 0,     /// 操作成功，watch继续生效
    kError,      /// 请求失败, watch失效
    kExisted,    /// 节点已存在，create失败
    kNotExist,   /// 节点不存在，对于exist操作watch继续生效，其他操作均失效
    kDeleted,    /// 节点删除，watch失效
    kNotEmpty    /// 节点有子节点，Delete失败
};

typedef struct NodeStat
{
    int64_t czxid;
    int64_t mzxid;
    int64_t ctime;
    int64_t mtime;
    int32_t version;
    int32_t cversion;
    int32_t aversion;
    int64_t ephemeralOwner;
    int32_t dataLength;
    int32_t numChildren;
    int64_t pzxid;
} NodeStat;

typedef boost::function <void()> SessionTimeoutCallback;
typedef boost::function <void(ErrorCode, const std::string&)> CreateCallback;
typedef boost::function <void(ErrorCode, const std::string&)> DeleteCallback;
typedef boost::function <void(ErrorCode, const std::string&, const NodeStat*)> SetCallback;
typedef boost::function <void(ErrorCode, const std::string&, const std::string&)> GetCallback;
typedef boost::function <void(ErrorCode, const std::string&, const NodeStat*)> ExistCallback;
typedef boost::function <void(ErrorCode, const std::string&, const std::vector<std::string>&)> GetChildrenCallback;

struct WatchContext
{
    WatchContext(const std::string& path, bool watch)
        : path_(path), watch_(watch) {}

    const std::string path_;
    const bool watch_;
    CreateCallback createCb_;
    DeleteCallback deleteCb_;
    SetCallback setCb_;
    GetCallback getCb_;
    GetChildrenCallback getChildrenCb_;
    ExistCallback existCb_;
};

class Client : boost::noncopyable
{
public:
    Client();
    ~Client();

    /// @host 127.0.0.1:3000,127.0.0.1:3001,127.0.0.1:3002
    bool init(const std::string& host, 
              uint32_t sessionTimeout, 
              const SessionTimeoutCallback& cb = SessionTimeoutCallback());
    bool createPersistent(const std::string& path, const std::string& data, const CreateCallback& cb);
    bool createPersistentSequential(const std::string& path, const std::string& data, const CreateCallback& cb);
    bool createEphemeral(const std::string& path, const std::string& data, const CreateCallback& cb);
    bool createEphemeralSequential(const std::string& path, const std::string& data, const CreateCallback& cb);
    bool del(const std::string& path, const DeleteCallback& cb);
    bool set(const std::string& path, const std::string& data, const SetCallback& cb);
    bool get(const std::string& path, const GetCallback& cb, bool watch = false);
    bool getChildren(const std::string& path, const GetChildrenCallback cb, bool watch = false);
    bool exist(const std::string& path, const ExistCallback& cb, bool watch = false);

private:
    static void handleSessionWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx);
    static void handleCreateCompletion(int rc, const char* value, const void* ctx);
    static void handleDeleteCompletion(int rc, const void* ctx);
    static void handleSetCompletion(int rc, const struct Stat* stat, const void* ctx);
    static void handleGetCompletion(int rc, const char* value, int len, const struct Stat* stat, const void* ctx);
    static void handleGetWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx);
    static void handleGetChildrenCompletion(int rc, const struct String_vector* strings, const void* ctx);
    static void handleGetChildrenWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx);
    static void handleExistCompletion(int rc, const struct Stat* stat, const void* ctx);
    static void handleExistWatcher(zhandle_t* zh, int type, int state, const char* path, void* ctx);

    void handleSessionState(zhandle_t* zhandle, int state);
    void checkSessionStateThreadFunc();
    void defaultSessionTimeoutHandler();
    bool create(const std::string& path, 
                const std::string& data,
                int flag,
                const CreateCallback& cb);
    
    bool started_;
    base::CountDownLatch sessionLatch_;
    base::Thread checkSessionThread_;
    zhandle_t* zkHandle_;
    time_t sessionDisconnectMs_;
    mutable base::MutexLock sessionStateLock_;
    int sessionState_;
    uint32_t sessionTimeout_;
    SessionTimeoutCallback sessionTimeoutCb_;
};

} // namespace zk
} // namespace plugins

#endif // PLUGINS_ZK_CLIENT_H
