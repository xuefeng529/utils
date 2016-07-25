#include "plugins/ssdb_client/ConnectionPool.h"
#include "plugins/ssdb_client/SSDB_client.h"

namespace ssdb
{

ConnectionPtr ConnectionPool::get()
{
	ConnectionPtr conn;
	base::MutexLockGuard lock(lock_);
	if (freeConns_.empty())
	{
		conn.reset(Client::connect(ip_, port_));
	}
	else
	{
		conn = freeConns_.front();
		freeConns_.pop_front();
	}

	if (conn)
	{
		usedConns_.push_back(conn);
	}
	return conn;
}

void ConnectionPool::release(const ConnectionPtr& conn)
{
	base::MutexLockGuard lock(lock_);
	for (ConnectionList::iterator it = usedConns_.begin(); it != usedConns_.end(); ++it)
	{
		if (*it == conn)
		{
			usedConns_.erase(it);
			freeConns_.push_back(conn);
			break;
		}
	}
}

} // namespace ssdb
