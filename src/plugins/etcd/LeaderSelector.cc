#include "plugins/etcd/LeaderSelector.h"
#include "plugins/etcd/JsonHelper.h"
#include "base/StringUtil.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>

namespace plugins
{
namespace etcd
{

LeaderSelector::LeaderSelector(const std::string& hosts, 
                               int timeout,
                               const std::string& parentNode,
                               const std::string& value,
                               const TakeLeaderCallBack& cb)
    : leader_(false),
      electLatch_(1),
      watchThread_(new base::Thread(boost::bind(&LeaderSelector::watchThreadFunc, this))),
      timeout_(timeout),
      parentNode_(parentNode),
      value_(value),
      takeLeaderCb_(cb)
{
    base::StringUtil::split(hosts, ",", &hosts_);
    if (hosts_.empty())
    {
        LOG_ERROR << "hosts empty";
        sleep(3);
        abort();
    }
}

void LeaderSelector::start()
{
    assert(!watchThread_->started());
    watchThread_->start();
    electLatch_.wait();
    /// 确保watchThreadFunc里面的cli->get()已经发送，和所有的过期节点已经删除
    sleep(timeout_ + 1);
    HttpClientPtr cli(new plugins::curl::HttpClient(true));
    for (size_t i = 0; i < hosts_.size(); ++i %= hosts_.size())
    {
        char url[1024];
        snprintf(url, sizeof(url), "http://%s/v2/keys/%s", hosts_[i].c_str(), parentNode_.c_str());
        char data[1024];
        snprintf(data, sizeof(data), "value=%s&ttl=%d", value_.c_str(), timeout_);
        std::string response;
        /// 创建带有ttl的有序节点
        if (cli->post(url, data, 3, &response))
        {  
            LOG_INFO << "post response: " << response;
            std::string key;
            std::string value;
            plugins::etcd::JsonHelper::getPostNode(response, &key, &value);
            snprintf(url, sizeof(url), "http://%s/v2/keys%s", 
                     hosts_[i].c_str(), key.c_str());
            snprintf(data, sizeof(data), "ttl=%d&refresh=true&prevExist=true", timeout_);
            LOG_INFO << "put url: " << url << "?" <<data;
            /// 刷新节点ttl
            while (cli->put(url, data, 3, &response))
            {
                int sleepInvt = timeout_ / 2;
                sleepInvt = sleepInvt < 1 ? 1 : sleepInvt;
                sleep(sleepInvt);               
            }           
        } 
        else
        {
            LOG_WARN << url;
            sleep(3);
        }
    }
}

void LeaderSelector::watchThreadFunc()
{
    electLatch_.countDown();
    HttpClientPtr cli(new plugins::curl::HttpClient(true));
    for (size_t i = 0; i < hosts_.size(); ++i %= hosts_.size())
    {
        char url[1024];
        snprintf(url, sizeof(url), "http://%s/v2/keys/%s?wait=true&recursive=true",
                 hosts_[i].c_str(), parentNode_.c_str());
        LOG_INFO << "watching: " << url;
        std::string response;
        /// 监控目录下子节点变化
        while (cli->get(url, 0, &response))
        {
            onChildrenChange(cli);
        }

        LOG_WARN << "watching failed: " << url;
        sleep(3);
    }
}

void LeaderSelector::onChildrenChange(const HttpClientPtr& cli)
{
    for (size_t i = 0; i < hosts_.size(); ++i %= hosts_.size())
    {
        char url[1024];
        snprintf(url, sizeof(url), "http://%s/v2/keys/%s?sorted=true",
                 hosts_[i].c_str(), parentNode_.c_str());
        std::string response;
        if (cli->get(url, 3, &response))
        {
            LOG_INFO << "all sorted nodes: " << response;
            /// key: sortedNodes[i]
            /// value: sortedNodes[i+1]
            std::vector<std::string> sortedNodes;
            plugins::etcd::JsonHelper::getSortedNodes(response, &sortedNodes);
            for (size_t i = 0; i < sortedNodes.size(); i+=2)
            {
                LOG_INFO << "sorted key: " << sortedNodes[i] << " value: " << sortedNodes[i+1];
            }

            /// sortedNodes[0]是目录下最小的子键
            if (sortedNodes.size() < 2)
            {
                LOG_ERROR << "no exist sorted node";
                break;
            }
            
            if (sortedNodes[1] == value_)
            {
                if (!leader_)
                {
                    leader_ = true;
                    LOG_INFO << "I am leader: " << value_;
                }
                else
                {
                    LOG_INFO << "I am already leader: " << value_;
                }
            }
            else
            {
                LOG_INFO << "I am follower: " << value_;
            }
            
            break;
        }
        else
        {
            LOG_WARN << url;
            sleep(3);
        }
    }
}

} // namespace etcd
} // namespace plugins
