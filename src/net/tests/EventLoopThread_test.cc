#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include "base/CountDownLatch.h"
#include "base/StringUtil.h"

#include <boost/bind.hpp>
#include <iostream>

base::CountDownLatch g_latch(1);
int64_t g_count = 0;

void worker(const std::string& task)
{
	static int64_t n = 0;
	std::cout << n++ << ": " << task << std::endl;
	if (n >= g_count)
	{
		g_latch.countDown();
	}
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "Usage: count" << std::endl;
		abort();
	}

	const std::string kVal(1024, 'x');
	net::EventLoop mainLoop;
	g_count = base::StringUtil::strToUInt64(argv[1]);
	net::EventLoopThread loopThread;
	net::EventLoop* loop = loopThread.startLoop();
	for (int64_t i = 0; i < g_count; i++)
	{
		loop->runInLoop(boost::bind(worker, kVal));
	}
	
	g_latch.wait();
	mainLoop.loop();

	return 0;
}