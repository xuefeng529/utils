#include "plugins/mysql_client/ConnectionPool.h"

namespace plugin
{
namespace mysql
{

ConnectionPool::ConnectionPool(const std::string& host,
	                           uint16_t port,
		                       const std::string& user,
		                       const std::string& password,
		                       size_t maxConnections)
    : host_(host),
	  port_(port),
	  user_(user),
	  password_(password),
	  maxConnections_(maxConnections)
{
}

ConnectionPtr ConnectionPool::get()
{
	ConnectionPtr conn;
	base::MutexLockGuard lock(lock_);
	for (ConnectionList::iterator it = conns_.begin(); it != conns_.end(); ++it)
	{
		if (!(*it)->inUsed())
		{
			conn = *it;
			conn->setInUsed(true);
			break;
		}
	}
	
	if (!conn && conns_.size() < maxConnections_)
	{
		conn.reset(new Connection());
		if (conn->connect(host_, port_, user_, password_))
		{
			conn->setInUsed(true);
			conns_.push_back(conn);
		}
		else
		{
			conn.reset();
		}
	}
	
	return conn;
}
	
void ConnectionPool::release(const ConnectionPtr& conn)
{
	base::MutexLockGuard lock(lock_);
	conn->setInUsed(false); 
}

boost::scoped_ptr<ConnectionPool> theConnPool;

} // namespace mysql
} // namespace plugin
