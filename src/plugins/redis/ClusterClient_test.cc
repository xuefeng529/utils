#include "base/Logging.h"
#include "net/Buffer.h"
#include "plugins/redis/ClusterClient.h"

#include <boost/scoped_ptr.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

boost::scoped_ptr<redis::ClusterClient> theCli;

void testString()
{
	LOG_INFO << "begin to set keys";
	for (int i = 0; i < 10000; i++)
	{
		char key[64];
		snprintf(key, sizeof(key), "id_%d", i);
		char val[64];
		snprintf(val, sizeof(val), "%d", i);
		theCli->set(key, val);
	}
	LOG_INFO << "setting keys complete";

	LOG_INFO << "begin to get keys";
	while (true)
	{
		for (int i = 0; i < 10000; i++)
		{
			char key[64];
			snprintf(key, sizeof(key), "id_%d", i);
			std::string val;
			theCli->get(key, &val);
			if (val.empty())
			{
				LOG_ERROR << "not found key: " << key;
			}
		}
	}

	LOG_INFO << "getting keys complete";
}

int main(int argc, char* argv[])
{
    theCli.reset(new redis::ClusterClient());
	if (!theCli->connect(argv[1]))
	{
		LOG_ERROR << "connecting failed";
		return -1;
	}

	testString();
}
