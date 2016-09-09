#include "plugins/mysql_client/Query.h"
#include "plugins/mysql_client/Connection.h"

#include <iostream>
#include <time.h>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS /// PRIu64
#define __STDC_FORMAT_MACROS
#endif
#include "inttypes.h"

void test(const std::string& ip, uint16_t port, const std::string& user, const std::string& password)
{
	plugin::mysql::ConnectionPtr conn(new plugin::mysql::Connection());
	if (!conn->connect(ip, port, user, password))
	{
		std::cout << "Failed to connect" << std::endl;
		return;
	}

	char sql[1024];
	snprintf(sql, sizeof(sql), "REPLACE INTO eastmoney_push.test "
		"(user_id, stock_id, notice_flag, max_price, min_price, price_range, onoff, increment_id) "
		"VALUES ('%s', '%s', %s, %s, %s, %s, %s, %" PRIu64 ")",
		"19810630", "123", "1", "20", "15", "10", "1", static_cast<int64_t>(8888));
	plugin::mysql::Query dbQuery(conn);
	if (!dbQuery.execute(sql))
	{
		std::cout << "Update database failed" << std::endl;
		return;
	}

	snprintf(sql, sizeof(sql), "REPLACE INTO eastmoney_push.test "
		"(user_id, stock_id, onoff, increment_id) "
		"VALUES ('%s', '%s', %s, %" PRIu64 ")",
		"19810630", "123", "1", static_cast<int64_t>(9999));
	if (!dbQuery.execute(sql))
	{
		std::cout << "Update database failed" << std::endl;
		return;
	}

	snprintf(sql, sizeof(sql), "SELECT * FROM eastmoney_push.test WHERE user_id='19810630'");
	if (!dbQuery.executeQuery(sql))
	{
		std::cout << "Update database failed" << std::endl;
		return;
	}

	for (size_t i = 0; i < dbQuery.rowsCount(); i++)
	{
		const plugin::mysql::Row* row = dbQuery.getRow(i);
		for (size_t n = 0; n < row->fieldsCount(); n++)
		{
			std::cout << row->toString(n) << " ";
		}
		std::cout << std::endl;
	}
}

int main(int argc, char* argv[])
{
	if (argc > 2)
	{
		/*if (!plugin::mysql::initDefaultConnectionPool(argv[1], static_cast<uint16_t>(atoi(argv[2])), "xf", "push@123"))
		{
			std::cout << "mysql init failed." << std::endl;
			abort();
		}*/
		
		test(argv[1], atoi(argv[2]), "xf", "push@123");
		return 0;
		plugin::mysql::Query query;
		/*int32_t no = static_cast<int32_t>(time(NULL));
		for (int i = 0; i < 100000; i++)
		{
		char buf[1024];
		snprintf(buf, sizeof(buf), "insert into test (title,author) values('title%d','author%d')", no, no);
		no++;
		if (!query.execute(buf))
		{
		std::cout << "query.execute fatal." << std::endl;
		abort();
		}
		}*/

		std::cout << "no store." << std::endl;
		query.executeQuery("select * from test", false);
		const plugin::mysql::Row* row;
		while ((row = query.fetchRow()))
		{
			std::cout << "field count:" << row->fieldsCount() << std::endl;
			for (size_t i = 0; i < row->fieldsCount(); ++i)
			{
				std::cout << row->toString(i) << " ";
			}
			std::cout << std::endl;
		}

		std::cout << "store." << std::endl;

		query.executeQuery("select * from test");
		size_t rowsCount = query.rowsCount();
		std::cout << "rowsCount:" << rowsCount << std::endl;
		for (size_t i = 0; i < rowsCount; ++i)
		{
			row = query.getRow(i);
			std::cout << "field count:" << row->fieldsCount() << std::endl;
			for (size_t i = 0; i < row->fieldsCount(); ++i)
			{
				std::cout << row->toString(i) << " ";
			}
			std::cout << std::endl;
		}
	}
	return 0;
}