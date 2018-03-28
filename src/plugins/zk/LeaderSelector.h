#ifndef PLUGINS_ZK_LEADERSELECTOR_H
#define PLUGINS_ZK_LEADERSELECTOR_H

#include "plugins/zk/Client.h"

#include <boost/scoped_ptr.hpp>

namespace plugins
{
namespace zk 
{

typedef boost::function<void()> TakeLeaderCallBack;

class LeaderSelector : boost::noncopyable
{
public:
    /// @host 127.0.0.1:3000,127.0.0.1:3001,127.0.0.1:3002
    /// @sessionTimeout 会话过期时间
    /// @parentNode 父节点 /parentNode
    /// @name 唯一标识
    LeaderSelector(const std::string& host, 
                   uint32_t sessionTimeout,
                   const std::string& parentNode,
                   const std::string& name,
                   const TakeLeaderCallBack& cb);

    void start();

private:
    void handleCreate(ErrorCode code, const std::string& path);
    void handleGetChildren(ErrorCode code, const std::string& path, const std::vector<std::string>& children);
    void handleGet(ErrorCode code, const std::string& path, const std::string& value);

    bool leader_;
    boost::scoped_ptr<Client> client_;
    const std::string host_;
    const uint32_t sessionTimeout_;
    const std::string parentNode_;
    const std::string subNode_;
    const std::string name_;
    const TakeLeaderCallBack takeLeaderCb_;
};


} // namespace zk
} // namespace plugins

#endif // PLUGINS_ZK_LEADERSELECTOR_H
