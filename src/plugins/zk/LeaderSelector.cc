#include "plugins/zk/LeaderSelector.h"
#include "base/Logging.h"

#include <vector>
#include <algorithm>
#include <boost/bind.hpp>
#include <assert.h>

namespace plugins
{
namespace zk
{

LeaderSelector::LeaderSelector(const std::string& host,
                               uint32_t sessionTimeout,
                               const std::string& parentNode,
                               const std::string& name,
                               const TakeLeaderCallBack& cb)
    : leader_(false),
      client_(new Client()),
      host_(host),
      sessionTimeout_(sessionTimeout),
      parentNode_(parentNode),
      subNode_(parentNode + "/sub-"),
      name_(name),
      takeLeaderCb_(cb)
{
}

void LeaderSelector::start()
{
	while (!client_->init(host_, sessionTimeout_, boost::bind(&LeaderSelector::handleSessionTimeout, this), kZkError))
    {
        sleep(3);
    }

    client_->createEphemeralSequential(subNode_, name_, boost::bind(&LeaderSelector::handleCreate, this, _1, _2));
}

void LeaderSelector::handleCreate(ErrorCode code, const std::string& path)
{
    if (code == kOk)
    {
		LOG_INFO << "@LeaderSelector: create node successfully, path: " << path;
        /// 不需要检查返回值，除非SESSION_EXPIRED才会失败，那也会被回调退出程序
        client_->getChildren(parentNode_, boost::bind(&LeaderSelector::handleGetChildren, this, _1, _2, _3), true);
    }
    else if (code == kError)
    {
        client_->createEphemeralSequential(subNode_, name_, boost::bind(&LeaderSelector::handleCreate, this, _1, _2));
    }
    else if (code == kNotExist)
    {
        LOG_ERROR << "@LeaderSelector: no parent node: " << parentNode_;
        sleep(3);
        exit(0);
    }
    else
    {
        LOG_WARN << "@LeaderSelector: the node already exists, path: " << path;
    }
}

void LeaderSelector::handleGetChildren(ErrorCode code, const std::string& path, const std::vector<std::string>& children)
{
    if (code == kOk)
    {
        if (!leader_)
        {
            /// 在follower状态才需要关注leader变化，在leader状态无需关注
			if (children.empty())
			{
				return;
			}
			
			for (size_t i = 0; i < children.size(); ++i)
			{
				LOG_INFO << "@LeaderSelector: child node: " << children[i];
			}
			
            std::vector<std::string> nodes(children);
            std::sort(nodes.begin(), nodes.end());
            std::string leaderNode = parentNode_ + "/" + nodes[0];
            /// 获取leader节点的value，与self_比较确认是否自己成为leader。
            client_->get(leaderNode, boost::bind(&LeaderSelector::handleGet, this, _1, _2, _3));
        }
        else 
        {
            LOG_INFO << "@LeaderSelector: I am already leader, just new node found - count=" << children.size();
        }
    }
    else if (code == kError)
    {
        client_->getChildren(parentNode_, boost::bind(&LeaderSelector::handleGetChildren, this, _1, _2, _3), true);
    }
}

void LeaderSelector::handleGet(ErrorCode code, const std::string& path, const std::string& value)
{
    if (code == kOk) 
    {
		LOG_INFO << "@LeaderSelector: got node, path: " << path << ", value: " << value;
        if (!leader_ && value == name_) 
        { 
            LOG_INFO << "@LeaderSelector: I am leader, path: " << path << ", value: " << value;
            leader_ = true;
            if (takeLeaderCb_)
            {
                takeLeaderCb_();
            }
        }
        else if (!leader_)
        { 
            LOG_INFO << "@LeaderSelector: I am follower, path: " << path << ", value: " << value;
        }
    }
    else if (code == kNotExist)
    {
        /// leader节点下线了，没关系，GetChildren一定会触发watch通知我们这个变化
    }
    else 
    {
        client_->get(path, boost::bind(&LeaderSelector::handleGet, this, _1, _2, _3));
    }
}

void LeaderSelector::handleSessionTimeout()
{
	LOG_INFO << "@LeaderSelector: handle session timeout";
	leader_ = false;
	while (!client_->reconnect())
	{
		sleep(3);
	}
	client_->createEphemeralSequential(subNode_, name_, boost::bind(&LeaderSelector::handleCreate, this, _1, _2));
}

} // namespace zk
} // namespace plugins
