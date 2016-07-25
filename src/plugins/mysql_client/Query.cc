#include "plugins/mysql_client/Query.h"
#include "plugins/mysql_client/ConnectionPool.h"
#include "plugins/mysql_client/Connection.h"
#include "base/Logging.h"

#include <stdio.h>
#include <stdlib.h>

namespace plugin
{
namespace mysql
{

bool initDefaultConnectionPool(const std::string& host,
							   uint16_t port,
							   const std::string& db,
							   const std::string& user,
							   const std::string& password,
							   size_t maxCount)
{
	MYSQL mysql;
	mysql_init(&mysql);
	if (mysql_real_connect(&mysql,
		host.c_str(), user.c_str(), password.c_str(), db.c_str(), port, NULL, 0) == NULL)
	{
		LOG_ERROR << "mysql_real_connect error " << mysql_errno(&mysql) << ": "
			<< mysql_error(&mysql);
		return false;
	}
	
	mysql_close(&mysql);
	theConnPool.reset(new ConnectionPool(host, port, db, user, password, maxCount));
	return true;
}

Query::Query()
	: conn_(theConnPool->get()),
	  result_(NULL),
	  rowsCount_(0),
	  fieldsCount_(0)
{
}

Query::Query(const ConnectionPoolPtr& connPool)
	: connPool_(connPool),
	  conn_(connPool_->get()),
	  result_(NULL),
	  rowsCount_(0),
	  fieldsCount_(0)
{

}

Query::~Query()
{
	if (result_ != NULL)
	{
		mysql_free_result(result_);
	}
	
	if (conn_)
	{
		if (connPool_)
		{
			connPool_->release(conn_);
		}
		else if (theConnPool)
		{
			theConnPool->release(conn_);
		}
	}
}

bool Query::execute(const char* sql)
{
	if (!conn_)
	{
		LOG_ERROR << "Query::execute: conn_ == NULL";
		return false;
	}
	
	clear();
	MYSQL *mysql = conn_->getMySql();
	assert(mysql != NULL);
	if (mysql_query(mysql, sql) != 0)
	{
		LOG_ERROR << "mysql_query of Query::execute error " << mysql_errno(mysql) << ": "
			<< mysql_error(mysql);
		return false;
	}

	return true;
}

bool Query::execute(const std::string& sql)
{
	return execute(sql.c_str());
}

bool Query::executeQuery(const char* sql, bool stored)
{
	if (!conn_)
	{
		LOG_ERROR << "Query::execute: conn_ == NULL";
		return false;
	}
	
	clear();
	MYSQL* mysql = conn_->getMySql();
	assert(mysql != NULL);
	if (mysql_query(mysql, sql) != 0)
	{
		LOG_ERROR << "mysql_query of Query::executeQuery error " << mysql_errno(mysql) << ": "
			<< mysql_error(mysql);
		return false;
	}
	
	result_ = ((stored_ = stored) ? mysql_store_result(mysql) : mysql_use_result(mysql));
	if (result_ == NULL)
	{
		if (stored_)
		{
			LOG_ERROR << "mysql_store_result of Query::executeQuery error "
				<< mysql_errno(mysql) << ": " << mysql_error(mysql);
		}
		else
		{
			LOG_ERROR << "mysql_use_result of Query::executeQuery error "
				<< mysql_errno(mysql) << ": " << mysql_error(mysql);
		}
		return false;
	}
	
	fieldsCount_ = mysql_num_fields(result_);
	MYSQL_FIELD* field = NULL;
	int index = 0;
	while ((field = mysql_fetch_field(result_)) != NULL)
	{
		nameIndexMap_.insert(std::make_pair(field->name, index++));
	}

	if (stored)
	{
		storeResult();
	}
	return true;
}

bool Query::executeQuery(const std::string& sql, bool stored)
{
	return executeQuery(sql.c_str(), stored);
}

void Query::storeResult()
{
	rowsCount_ = mysql_num_rows(result_);
	MYSQL_ROW row = NULL;
	while ((row = mysql_fetch_row(result_)) != NULL)
	{
		rows_.push_back(Row(row, &nameIndexMap_));
	}
}

const Row* Query::getRow(uint32_t index) const
{
	if (stored_ && index < rowsCount_)
	{
		return &rows_[index];
	}
	return NULL;
}

const Row* Query::fetchRow()
{
	if (!stored_)
	{
		MYSQL_ROW row = mysql_fetch_row(result_);
		if (row == NULL)
		{
			return NULL;
		}

		Row tmp(row, &nameIndexMap_);
		row_.swap(tmp);
		return &row_;
	}
	return NULL;
}

void Query::clear()
{
	if (result_ != NULL)
	{
		mysql_free_result(result_);
		result_ = NULL;
	}

	stored_ = false;
	rows_.clear();
	nameIndexMap_.clear();
	rowsCount_ = 0;
	fieldsCount_ = 0;
}

} // namespace mysql
} // namespace plugin
