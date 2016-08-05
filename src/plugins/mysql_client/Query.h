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

/// �������ݿ��Ƿ���ͨ
/// @host: ��Ҫ���ӵ����ݿ�IP��ַ
/// @port: ��Ҫ���ӵ����ݿ����˿ں�
/// @db: ��Ҫ���ӵ����ݿ���
/// @user: �������ݿ��õ��û���
/// @password: �������ݿ��õ�����
/// @return: �ɹ�true,ʧ��false
bool ping(const std::string& host,
		  uint16_t port,
		  const std::string& db,
		  const std::string& user,
		  const std::string& password);

/// ��ʼ�����ݿ�
/// @maxSize: ���ݿ����������
/// @host: ��Ҫ���ӵ����ݿ�IP��ַ
/// @port: ��Ҫ���ӵ����ݿ����˿ں�
/// @db: ��Ҫ���ӵ����ݿ���
/// @user: �������ݿ��õ��û���
/// @password: �������ݿ��õ�����
/// @return: �ɹ�true,ʧ��false
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

	/// ���ݿ�insert��update���²���
	/// @return: �ɹ�true,ʧ��false
	bool execute(const char* sql);
	bool execute(const std::string& sql);
	
	/// ���ݿ��ѯ�������������select, show, describe, explain��check table��
	/// @isStored: �Ƿ�ѱ��β�ѯ������浽����
	/// @return: �ɹ�true,ʧ��false
	bool executeQuery(const char* sql, bool stored = true);
	bool executeQuery(const std::string& sql, bool stored = true);

	size_t rowsCount() const
	{ return rowsCount_; }

	const Row* getRow(uint32_t index) const;

	/// ����select�������صĽ��
	/// @return: ��NULL��¼�У�NULL�������
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
