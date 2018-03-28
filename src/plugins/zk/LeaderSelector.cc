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
    while (!client_->init(host_, sessionTimeout_))
    {
        sleep(1);
    }

    client_->createEphemeralSequential(subNode_, name_, boost::bind(&LeaderSelector::handleCreate, this, _1, _2));
}

void LeaderSelector::handleCreate(ErrorCode code, const std::string& path)
{
    if (code == kOk)
    {
        LOG_INFO << path << " [handleCreate]";
        /// ����Ҫ��鷵��ֵ������SESSION_EXPIRED�Ż�ʧ�ܣ���Ҳ�ᱻ�ص��˳�����
        client_->getChildren(parentNode_, boost::bind(&LeaderSelector::handleGetChildren, this, _1, _2, _3), true);
    }
    else if (code == kError)
    {
        client_->createEphemeralSequential(subNode_, name_, boost::bind(&LeaderSelector::handleCreate, this, _1, _2));
    }
    else if (code == kNotExist)
    {
        LOG_ERROR << "You need to create " << parentNode_;
        sleep(3);
        exit(0);
    }
    else
    {
        LOG_WARN << "kExist " << path;
    }
}

void LeaderSelector::handleGetChildren(ErrorCode code, const std::string& path, const std::vector<std::string>& children)
{
    if (code == kOk)
    {
        if (!leader_)
        {
            /// ��follower״̬����Ҫ��עleader�仯����leader״̬�����ע
            assert(!children.empty());
            std::vector<std::string> nodes(children);
            std::sort(nodes.begin(), nodes.end());
            std::string leaderNode = parentNode_ + "/" + nodes[0];
            /// ��ȡleader�ڵ��value����self_�Ƚ�ȷ���Ƿ��Լ���Ϊleader��
            client_->get(leaderNode, boost::bind(&LeaderSelector::handleGet, this, _1, _2, _3));
        }
        else 
        {
            LOG_INFO << "I am already Leader, just new node found - count=" << children.size();
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
        if (!leader_ && value == name_) 
        { 
            LOG_INFO << "I am leader - " << path << " " << value;
            leader_ = true;
            if (takeLeaderCb_)
            {
                takeLeaderCb_();
            }
        }
        else if (!leader_)
        { 
            LOG_INFO << "I am follower - " << path << " " << value;
        }
    }
    else if (code == kNotExist)
    {
        /// leader�ڵ������ˣ�û��ϵ��GetChildrenһ���ᴥ��watch֪ͨ��������仯
    }
    else 
    {
        client_->get(path, boost::bind(&LeaderSelector::handleGet, this, _1, _2, _3));
    }
}

} // namespace zk
} // namespace plugins
