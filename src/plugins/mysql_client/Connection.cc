#include "base/Logging.h"
#include "plugins/mysql_client/Connection.h"

#include <string.h>

namespace plugin
{
namespace mysql
{

bool Connection::connect(const std::string& host,
						 uint16_t port,
						 const std::string& db,
						 const std::string& user,
						 const std::string& password)
{
	mysql_init(&mysql_);

	my_bool autoReconnect = 1; ///< 设置自动重连接
	mysql_options(&mysql_, MYSQL_OPT_RECONNECT, &autoReconnect);

	if (mysql_real_connect(&mysql_,
		host.c_str(), user.c_str(), password.c_str(), db.c_str(), port, NULL, 0) == NULL)
	{
		LOG_ERROR << "mysql_real_connect of  Connection::connect error " << mysql_errno(&mysql_) << ": "
			<< mysql_error(&mysql_);
		connected_ = false;
		return false;
	}
	
	mysql_set_character_set(&mysql_, "utf8");
	connected_ = true;
	return true;
}

void Connection::disconnect()
{
	if (connected_)
	{
		mysql_close(&mysql_);
		memset(&mysql_, 0, sizeof(mysql_));
		connected_ = false;
	}
}

} // namespace mysql
} // namespace plugin
