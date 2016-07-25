#ifndef PLUGIN_MYSQL_CLIENT_CONNECTION_H
#define PLUGIN_MYSQL_CLIENT_CONNECTION_H

#include <mysql.h>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

#include <stdint.h>

namespace plugin
{
namespace mysql
{

class Connection : boost::noncopyable
{
public:
	Connection() : connected_(false) { }
	~Connection() { disconnect(); }

	bool connect(const std::string& host,
				 uint16_t port, 
				 const std::string& db,
				 const std::string& user,
				 const std::string& password);

	void disconnect();

	bool connected() const
	{ return connected_; }

	MYSQL* getMySql()
	{ return &mysql_; }

	void setInUsed(bool inUsed)
	{ inUsed_ = inUsed; }

	bool inUsed() const
	{ return inUsed_; }
	
private:
	MYSQL mysql_;
	bool connected_;
	bool inUsed_;
};

} // namespace mysql
} // namespace plugin

#endif // PLUGIN_MYSQL_CLIENT_CONNECTION_H
