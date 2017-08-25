#ifndef PLUGIN_ZK_ZKLOCK_H
#define PLUGIN_ZK_ZKLOCK_H

#include <boost/noncopyable.hpp>
#include <string>

namespace zk
{

class ZkLock : boost::noncopyable
{
public:
    ZkLock(const std::string& lockPath);
    void lock();
    void unlock();

private:
};

} // namespace zk

#endif // PLUGIN_ZK_ZKLOCK_H
