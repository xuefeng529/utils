#include "plugins/mysql_client/Query.h"

#include <iostream>
#include <time.h>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS /// PRIu64
#define __STDC_FORMAT_MACROS
#endif
#include "inttypes.h"

int main(int argc, char* argv[])
{
	if (argc > 2)
	{
		if (!plugin::mysql::initDefaultConnectionPool(argv[1], atoi(argv[2]), "eastmoney_push", "xf", "push@123"))
		{
			std::cout << "mysql init failed." << std::endl;
			abort();
		}

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