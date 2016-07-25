#include "plugins/ssdb_client/SSDB_client.h"

#include <boost/scoped_ptr.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
	const char *ip = (argc >= 2) ? argv[1] : "127.0.0.1";
	int port = (argc >= 3) ? atoi(argv[2]) : 8888;

	boost::scoped_ptr<ssdb::Client> client(ssdb::Client::connect(ip, port));
	if (!client){
		printf("fail to connect to server!\n");
		return 0;
	}

	ssdb::Status s;
	s = client->hset("test", "111111", "xf");
	if (!s.ok())
	{
		printf("error!\n");
		return 0;
	}

	s = client->hset("test", "222222", "zh");
	if (!s.ok())
	{
		printf("error!\n");
		return 0;
	}
	
	std::string val;
	s = client->hget("test", "111111", &val);
	if (s.ok())
	{
		printf("111111 : %s\n", val.c_str());
	}
	else
	{
		printf("111111 have not val.\n");
	}

	s = client->hget("test", "222222", &val);
	if (s.ok())
	{
		printf("222222 : %s\n", val.c_str());
	}
	else
	{
		printf("222222 have not val.\n");
	}

	s = client->hget("test", "333333", &val);
	if (s.ok())
	{
		printf("333333 : %s\n", val.c_str());
	}
	else
	{
		printf("333333 have not val.\n");
	}

	return 0;
}