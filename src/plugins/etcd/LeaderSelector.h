#ifndef PLUGINS_ETCD_LEADERSELECTOR_H
#define PLUGINS_ETCD_LEADERSELECTOR_H

#include "base/Thread.h"
#include "base/Atomic.h"
#include "base/CountDownLatch.h"
#include "plugins/curl/HttpClient.h"

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

namespace plugins
{
namespace etcd 
{

typedef enum { kLeader, kFollower } ElectionType;
typedef boost::shared_ptr<plugins::curl::HttpClient> HttpClientPtr;
typedef boost::function<void(ElectionType)> LeaderElectionCallBack;

class LeaderSelector : boost::noncopyable
{
public:
    /// @host 127.0.0.1:2379,127.0.0.2:2379,127.0.0.3:2379
    /// @timeout �Ự����ʱ��
    /// @parentNode ���ڵ� /parentNode
    /// @value Ψһ��ʶ
    LeaderSelector(const std::string& hosts, 
                   int timeout,
                   const std::string& parentNode,
                   const std::string& value,
                   const LeaderElectionCallBack& cb);

    void start();

private:
    void delay(int seconds);
    void watchThreadFunc();
    void refreshThreadFunc();
    bool onChildrenChange(const HttpClientPtr& cli, const std::string& host);
    
    bool started_;
    bool leader_;
    boost::scoped_ptr<base::Thread> watchThread_;
    boost::scoped_ptr<base::Thread> refreshThread_;
    base::CountDownLatch initElectionLatch_;
    std::vector<std::string> hosts_;
    const int timeout_;
    const std::string parentNode_;
    const std::string value_;
    const LeaderElectionCallBack leaderElectionCb_;
    base::AtomicInt32 ownNodeCreated_;
};


} // namespace etcd
} // namespace plugins

#endif // PLUGINS_ETCD_LEADERSELECTOR_H
