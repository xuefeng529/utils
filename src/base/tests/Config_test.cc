#include "base/Config.h"

#include <stdio.h>


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		fprintf(stderr, "%s filename\n", argv[0]);
		return 1;
	}

	base::Config config;
	if (!config.load(argv[1]))
	{
		fprintf(stderr, "load config failure: %s\n", argv[1]);
		return -1;
	}

	std::vector<std::string> sections;
	config.getSectionNames(&sections);
	for (size_t i = 0; i < sections.size(); i++)
	{
		if (sections[i] == "cache_server")
		{
			std::vector<std::string> servers;
			config.getStringList(sections[i], "server", &servers);
			for (size_t j = 0; j < servers.size(); j++)
			{
				fprintf(stdout, "server=%s\n", servers[j].c_str());
			}
		}
	}

	{
		int32_t port;
		config.getInt32("user_config_server", "port", &port);
		fprintf(stdout, "[user_config_server]\n");
		fprintf(stdout, "port = %d\n", port);
	}
	
	{
		std::string name, user, passwd;
		config.getString("db", "name", &name);
		config.getString("db", "user", &user);
		config.getString("db", "passwd", &passwd);
		fprintf(stdout, "[db]\n");
		fprintf(stdout, "name = %s\n", name.c_str());
		fprintf(stdout, "user = %s\n", user.c_str());
		fprintf(stdout, "passwd = %s\n", passwd.c_str());
	}
}