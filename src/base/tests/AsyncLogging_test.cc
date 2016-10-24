#include "base/AsyncLogging.h"
#include "base/Logging.h"
#include "base/FileUtil.h"
#include "base/daemon.h"
#include "base/StringUtil.h"

#include <boost/scoped_ptr.hpp>

#include <time.h>

boost::scoped_ptr<base::AsyncLogging> g_asyncLog;

void asyncOutput(const char* msg, int len)
{
	g_asyncLog->append(msg, len);
}

void initLog(int argc, char* argv[])
{
	base::Logger::setOutput(asyncOutput);
	g_asyncLog.reset(new base::AsyncLogging(
		base::StringUtil::extractDirname(argv[0]), base::StringUtil::extractFilename(argv[0]), 1024 * 1024 * 1024));
	base::Logger::setLogLevel(base::Logger::INFO);
	g_asyncLog->start();
}

int main(int argc, char* argv[])
{
	initLog(argc, argv);

	char buf[1024];
	int i = 0;
	while (true)
	{
		snprintf(buf, sizeof(buf), "Logging test %d", i++);
		LOG_INFO << buf;
		struct timespec ts = { 0, 500 * 1000 * 1000 };
		nanosleep(&ts, NULL);
	}

	return 0;
}