#ifndef PLUGIN_MYSQL_CLIENT_CONNECTION_POOL_H
#define PLUGIN_MYSQL_CLIENT_CONNECTION_POOL_H

#include "base/Mutex.h"
#include "plugins/mysql_client/Connection.h"

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <list>

namespace plugin
{
namespace mysql
{

class ConnectionPool : boost::noncopyable
{
public:
	ConnectionPool(const std::string& host,
			       uint16_t port,
				   const std::string& user,
				   const std::string& password,
		           size_t maxConnections)
	: host_(host),
	  port_(port),
	  user_(user),
	  password_(password),
	  maxConnections_(maxConnections)
	{ }

	ConnectionPtr get();
	void release(const ConnectionPtr& conn);
	const std::string& host() const
	{ return host_; }
	
private:
	typedef std::list<ConnectionPtr> ConnectionList;

	std::string host_;
	uint16_t port_;
	std::string user_;
	std::string password_;
	size_t maxConnections_;

	base::MutexLock lock_;
	ConnectionList conns_;
};

typedef boost::shared_ptr<ConnectionPool> ConnectionPoolPtr;

extern boost::scoped_ptr<ConnectionPool> theConnPool;

} // namespace mysql
} // namespace plugin

#endif // PLUGIN_MYSQL_CLIENT_CONNECTION_POOL_H
