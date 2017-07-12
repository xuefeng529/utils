#include "net/EventLoop.h"
#include "base/Thread.h"

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

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

int main()
{
  printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

  printf("begin to runInLoop\n");
  loop.runInLoop(boost::bind(runInLoopCallback, "run in loop"));

  printf("begin to runAfter\n");
  loop.runAfter(3, boost::bind(afterCallback, "after 3 seconds"));

  printf("begin to runEvery\n");
  loop.runEvery(3, boost::bind(everyCallback, &loop));

  //Thread thread(boost::bind(threadFunc, &loop));
  //thread.start();

  printf("begin to loop\n");
  loop.loop();

  printf("exit loop\n");
}
