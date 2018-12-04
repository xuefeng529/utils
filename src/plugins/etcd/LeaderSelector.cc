#include "plugins/etcd/LeaderSelector.h"
#include "plugins/etcd/JsonHelper.h"
#include "base/StringUtil.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <sys/select.h>

namespace plugins
{
namespace etcd
{

const std::string kEtcdIndexHeader = "X-Etcd-Index";

LeaderSelector::LeaderSelector(const std::string& hosts, 
                               int timeout,
                               const std::string& parentNode,
                               const std::string& value,
                               const LeaderElectionCallBack& cb)
    : started_(false),
      leader_(false),
      watchThread_(new base::Thread(boost::bind(&LeaderSelector::watchThreadFunc, this))),
      refreshThread_(new base::Thread(boost::bind(&LeaderSelector::refreshThreadFunc, this))),
      initElectionLatch_(1),
      timeout_(timeout),
      parentNode_(parentNode),
      value_(value),
      leaderElectionCb_(cb)
{
    base::StringUtil::split(hosts, ",", &hosts_);
    if (hosts_.empty())
    {
        LOG_ERROR << "hosts empty: " << hosts;
        sleep(3);
        abort();
    }

    for (size_t i = 0; i < hosts_.size(); i++)
    {
        LOG_INFO << "host: " << hosts_[i];
    }   
}

void LeaderSelector::start()
{
    assert(!started_);
    started_ = true;
    watchThread_->start();
    refreshThread_->start();
    initElectionLatch_.wait();
}

void LeaderSelector::delay(int seconds)
{
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    int ret;
    do
    {
        ret = select(0, NULL, NULL, NULL, &tv);
    } while (ret < 0 && errno == EINTR);
}

void LeaderSelector::watchThreadFunc()
{  
    HttpClientPtr cli(new plugins::curl::HttpClient(true));
    for (size_t i = 0; i < hosts_.size(); ++i %= hosts_.size())
    {
        std::string response;
        std::map<std::string, std::string> headers;
        char url[1024];       
        snprintf(url, sizeof(url), "http://%s/v2/keys/%s?wait=true&recursive=true",
                 hosts_[i].c_str(), parentNode_.c_str());
        LOG_INFO << "watching: " << url;      
        /// 监控目录下子节点变化
        do 
        {
            if (!cli->get(url, timeout_, &response))
            {
                /// 28 timeout
                if (cli->statusCode() != 28)
                {
                    break;
                }
                
                if (ownNodeCreated_.get() > 0)
                {
                    if (!onChildrenChange(cli, hosts_[i]))
                    {
                        break;
                    }

                    ownNodeCreated_.decrement();
                }  
            }
            else
            {
                if (!onChildrenChange(cli, hosts_[i]))
                {
                    break;
                }

                ownNodeCreated_.decrement();
            }
            
        } while (true);
        
        LOG_WARN << "watching failed: " << url;
        delay(timeout_);
    }
}

void LeaderSelector::refreshThreadFunc()
{
    HttpClientPtr cli(new plugins::curl::HttpClient(true));
    for (size_t i = 0; i < hosts_.size(); ++i %= hosts_.size())
    {      
        char url[1024];
        snprintf(url, sizeof(url), "http://%s/v2/keys/%s", hosts_[i].c_str(), parentNode_.c_str());
        char data[1024];
        snprintf(data, sizeof(data), "value=%s&ttl=%d", value_.c_str(), timeout_);
        std::string response;
        /// 创建带有ttl的有序节点
        if (!cli->post(url, data, timeout_, &response))
        {
            LOG_WARN << "post failed: " << url;
            delay(timeout_);
            continue;
        }

        LOG_INFO << "post response: " << response;
        ownNodeCreated_.increment();
        std::string key;
        std::string value;
        plugins::etcd::JsonHelper::getPostNode(response, &key, &value);
        snprintf(url, sizeof(url), "http://%s/v2/keys%s",
                 hosts_[i].c_str(), key.c_str());
        snprintf(data, sizeof(data), "ttl=%d&refresh=true&prevExist=true", timeout_);
        LOG_INFO << "put url: " << url << "?" << data;
        /// 刷新节点ttl
        while (cli->put(url, data, timeout_, &response))
        {
            int sleepInvt = timeout_ / 2;
            sleepInvt = sleepInvt < 1 ? 1 : sleepInvt;
            delay(sleepInvt);
        }

        delay(timeout_);
    }
}

bool LeaderSelector::onChildrenChange(const HttpClientPtr& cli, const std::string& host)
{
    char url[1024];
    snprintf(url, sizeof(url), "http://%s/v2/keys/%s?sorted=true", host.c_str(), parentNode_.c_str());
    std::string response;
    if (!cli->get(url, timeout_, &response))
    {
        LOG_WARN << url;
        return false;
    }

    LOG_INFO << "all sorted nodes: " << response;
    /// key: sortedNodes[i]
    /// value: sortedNodes[i+1]
    std::vector<std::string> sortedNodes;
    plugins::etcd::JsonHelper::getSortedNodes(response, &sortedNodes);
    for (size_t i = 0; i < sortedNodes.size(); i += 2)
    {
        LOG_INFO << "sorted key: " << sortedNodes[i] << " value: " << sortedNodes[i + 1];
    }

    /// sortedNodes[0]是目录下最小的子键
    if (sortedNodes.size() < 2)
    {
        LOG_WARN << "no exist sorted node";
        return true;
    }

    if (sortedNodes[1] == value_)
    {
        if (!leader_)
        {
            LOG_INFO << "I am leader: " << value_;
            leader_ = true;
            if (leaderElectionCb_)
            {                
                leaderElectionCb_(kLeader);
            }
        }
        else
        {
            LOG_INFO << "I am already leader: " << value_;
        }
    }
    else
    {
        if (leader_)
        {
            LOG_INFO << "I am follower: " << value_;
            leader_ = false;
            if (leaderElectionCb_)
            {
                leaderElectionCb_(kFollower);
            }
        }
        else
        {
            LOG_INFO << "I am already follower: " << value_;
        }  
    }

    LOG_INFO << "election complete";
    initElectionLatch_.countDown();

    return true;
}

} // namespace etcd
} // namespace plugins
