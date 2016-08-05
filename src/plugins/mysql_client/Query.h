#ifndef PLUGIN_MYSQL_CLIENT_QUERY_H
#define PLUGIN_MYSQL_CLIENT_QUERY_H

#include "plugins/mysql_client/Row.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <map>

namespace plugin
{
namespace mysql
{

class Connection;
class ConnectionPool;
typedef boost::shared_ptr<ConnectionPool> ConnectionPoolPtr;

const size_t kMaxConnections = 16;

/// 测试数据库是否连通
/// @host: 需要连接的数据库IP地址
/// @port: 需要连接的数据库服务端口号
/// @db: 需要连接的数据库名
/// @user: 连接数据库用的用户名
/// @password: 连接数据库用的密码
/// @return: 成功true,失败false
bool ping(const std::string& host,
		  uint16_t port,
		  const std::string& db,
		  const std::string& user,
		  const std::string& password);

/// 初始化数据库
/// @maxSize: 数据库最大连接数
/// @host: 需要连接的数据库IP地址
/// @port: 需要连接的数据库服务端口号
/// @db: 需要连接的数据库名
/// @user: 连接数据库用的用户名
/// @password: 连接数据库用的密码
/// @return: 成功true,失败false
bool initDefaultConnectionPool(const std::string& host,
							   uint16_t port,
							   const std::string& db,
							   const std::string& user,
							   const std::string& password,
							   size_t maxConnections = kMaxConnections);

class Query : boost::noncopyable
{
public:
	Query();
	Query(const ConnectionPoolPtr& connPool);
	~Query();

	/// 数据库insert和update更新操作
	/// @return: 成功true,失败false
	bool execute(const char* sql);
	bool execute(const std::string& sql);
	
	/// 数据库查询类操作，包括：select, show, describe, explain和check table等
	/// @isStored: 是否把本次查询结果保存到本地
	/// @return: 成功true,失败false
	bool executeQuery(const char* sql, bool stored = true);
	bool executeQuery(const std::string& sql, bool stored = true);

	size_t rowsCount() const
	{ return rowsCount_; }

	const Row* getRow(uint32_t index) const;

	/// 遍历select操作返回的结果
	/// @return: 非NULL记录行，NULL遍历完毕
	const Row* fetchRow();

private:
	void clear();
	void storeResult();

	ConnectionPoolPtr connPool_;
	typedef boost::shared_ptr<Connection> ConnectionPtr;
	ConnectionPtr conn_;
	MYSQL_RES* result_;
	bool stored_;
	typedef std::map<std::string, uint32_t> FieldNameIndexMap;
	FieldNameIndexMap nameIndexMap_;
	std::vector<Row> rows_;
	size_t rowsCount_;
	size_t fieldsCount_;
	Row row_;
};

} // namespace mysql
} // namespace plugin

#endif // PLUGIN_MYSQL_CLIENT_QUERY_H
