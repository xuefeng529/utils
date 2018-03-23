#ifndef PLUGINS_ZK_LOCK_H
#define PLUGINS_ZK_LOCK_H

#include <boost/noncopyable.hpp>
#include <string>

namespace plugins
{
namespace zk
{

class Lock : boost::noncopyable
{
public:
    Lock(const std::string& lockPath);
    void lock();
    void unlock();
};

} // namespace zk
} // namespace plugins

#endif // PLUGINS_ZK_LOCK_H
