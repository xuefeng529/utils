#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include "base/Thread.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

int cnt = 0;

int64_t timerId = 0;

void printTid()
{
	printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
	printf("now %s\n", Timestamp::now().toString().c_str());
}

void myprint(net::EventLoop* loop, const std::string& msg)
{
	printf("%s: %s\n", Timestamp::now().toString().c_str(), msg.c_str());
    loop->runAfter(3, boost::bind(myprint, loop, msg));
    /*static int count = 0;
    if (++count == 5)
    {
        loop->cancel(timerId);
    }
    else
    {
        loop->runAfter(3, boost::bind(myprint, loop, msg));
    }*/

    //loop->runAfter(3, boost::bind(myprint, loop, msg));
    //loop->quit();
}

//void cancel(int64_t timerId)
//{
//	g_loop->cancel(timerId);
//	printf("cancelled at %s\n", Timestamp::now().toString().c_str());
//}

int main()
{
	printTid();
    sleep(1);

    {

        EventLoop loop;

        for (int i = 0; i < 3; i++)
        {
            char name[32];
            snprintf(name, sizeof(name), "timer_%d", i);
            timerId = loop.runAfter(3, boost::bind(myprint, &loop, std::string(name)));
        }

        //int64_t timeId = loop.runEvery(2, boost::bind(print, "every2"));
        //(void)timeId;
        //loop.runAfter(20, boost::bind(cancel, timeId));
        //loop.runAfter(22, boost::bind(quit));

        loop.loop();
        printf("main loop exits");
    }

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
