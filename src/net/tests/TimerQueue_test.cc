#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include "base/Thread.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

int cnt = 0;
EventLoop* g_loop;

void printTid()
{
	printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
	printf("now %s\n", Timestamp::now().toString().c_str());
}

void print(const std::string& msg)
{
	printf("%s: %s\n", Timestamp::now().toString().c_str(), msg.c_str());
	g_loop->runAfter(1, boost::bind(print, msg));
}

void cancel(int64_t timerId)
{
	g_loop->cancel(timerId);
	printf("cancelled at %s\n", Timestamp::now().toString().c_str());
}

void quit()
{
	printTid();
	g_loop->quit();
}

int main()
{
	printTid();
	sleep(1);
	
	EventLoop loop;
	g_loop = &loop;

	for (int i = 0; i < 10; i++)
	{
		char name[32];
		snprintf(name, sizeof(name), "timer_%d", i);
		loop.runAfter(1, boost::bind(print, std::string(name)));
	}

	//int64_t timeId = loop.runEvery(2, boost::bind(print, "every2"));
	//(void)timeId;
	//loop.runAfter(20, boost::bind(cancel, timeId));
	//loop.runAfter(22, boost::bind(quit));

	loop.loop();
	print("main loop exits");

	/*sleep(1);
	{
		EventLoopThread loopThread;
		EventLoop* loop = loopThread.startLoop();
		loop->runAfter(2, printTid);
		sleep(3);
		print("thread loop exits");
	}*/

	return 0;
}
