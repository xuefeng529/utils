#ifndef PLUGIN_SSDB_CONNECTIONPOOL_H
#define PLUGIN_SSDB_CONNECTIONPOOL_H

#include "base/Mutex.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <list>

namespace ssdb
{

class Client;

typedef boost::shared_ptr<Client> ConnectionPtr;

class ConnectionPool : boost::noncopyable
{
public:
	ConnectionPool(const std::string& ip, uint16_t port)
		: ip_(ip), port_(port)
	{ }

	ConnectionPtr get();
	void release(const ConnectionPtr& client);

private:
	typedef std::list<ConnectionPtr> ConnectionList;

	std::string ip_;
	uint16_t port_;
	base::MutexLock lock_;
	ConnectionList usedConns_;
	ConnectionList freeConns_;
};

} // namespace ssdb

#endif // PLUGIN_SSDB_CONNECTIONPOOL_H
