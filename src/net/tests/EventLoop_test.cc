#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include "base/Thread.h"
#include "base/Logging.h"
#include "base/Atomic.h"
#include "base/CountDownLatch.h"
#include "base/Timestamp.h"

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

base::AtomicInt32 theCount;
base::CountDownLatch theLatch(1);

void afterCallback(const std::string& v)
{
	printf("%s\n", v.c_str());
}

void runInLoopCallback(const std::string& v)
{
	printf("%s\n", v.c_str());
}

void everyCallback(EventLoop* loop)
{
  static int num = 0;
  printf("every callback: %d\n", num++);
  if (num >= 5)
  {
	  loop->quit();
  }
}

void handle(int i)
{
    int n = theCount.incrementAndGet();
    //LOG_INFO << i << "," << n;
    if (n == 80000000)
    {
        theLatch.countDown();
    }
}

void produceThread(net::EventLoop* loop)
{
    LOG_INFO << "begin to produce, tid = " << base::CurrentThread::tid();
    for (int i = 0; i < 10000000; i++)
    {
        loop->runInLoop(boost::bind(handle, i));
    }
    LOG_INFO << "end to produce, tid = " << base::CurrentThread::tid();
}

int main()
{
    net::EventLoopThread loopThread;
    net::EventLoop* loop = loopThread.startLoop();
    base::Thread produceThread1(boost::bind(produceThread, loop));
    base::Thread produceThread2(boost::bind(produceThread, loop));
    base::Thread produceThread3(boost::bind(produceThread, loop));
    base::Thread produceThread4(boost::bind(produceThread, loop));
    base::Thread produceThread5(boost::bind(produceThread, loop));
    base::Thread produceThread6(boost::bind(produceThread, loop));
    base::Thread produceThread7(boost::bind(produceThread, loop));
    base::Thread produceThread8(boost::bind(produceThread, loop));
   
    produceThread1.start();
    produceThread2.start();
    produceThread3.start();
    produceThread4.start();
    produceThread5.start();
    produceThread6.start();
    produceThread7.start();
    produceThread8.start();
    
    int64_t begin = base::Timestamp::now().microSecondsSinceEpoch();
    produceThread1.join();
    produceThread2.join();
    produceThread3.join();
    produceThread4.join();
    produceThread5.join();
    produceThread6.join();
    produceThread7.join();
    produceThread8.join();
    theLatch.wait();
    int64_t end = base::Timestamp::now().microSecondsSinceEpoch();
    LOG_INFO << "time: " << (end - begin) / 1000 << " ms";
    return 0;
  //printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  //assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  //EventLoop loop;
  //assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

  //printf("begin to runInLoop\n");
  //loop.runInLoop(boost::bind(runInLoopCallback, "run in loop"));

  //printf("begin to runAfter\n");
  //loop.runAfter(3, boost::bind(afterCallback, "after 3 seconds"));

  //printf("begin to runEvery\n");
  //loop.runEvery(3, boost::bind(everyCallback, &loop));

  ////Thread thread(boost::bind(threadFunc, &loop));
  ////thread.start();

  //printf("begin to loop\n");
  //loop.loop();

  //printf("exit loop\n");
}
