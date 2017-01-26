#include "net/EventLoop.h"
#include "base/Thread.h"

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

void callback(int n)
{
  printf("id = %d\n", n);
}

void threadFunc(EventLoop* loop)
{
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  int32_t num = 0;
  while (num < 5)
  {
	  loop->runInLoop(boost::bind(callback, num++));
	  sleep(1);
  }

  loop->quit();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

  Thread thread(boost::bind(threadFunc, &loop));
  thread.start();

  loop.loop();

  printf("exit loop\n");
}
