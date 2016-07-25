#include "net/EventLoop.h"
#include "base/Thread.h"

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

EventLoop* g_loop;

void callback(int id)
{
  printf("id = %d\n", id);
}

void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
  //loop.runAfter(1.0, callback);
  loop.runInLoop(boost::bind(callback, 1));
  loop.runInLoop(boost::bind(callback, 2));
  loop.runInLoop(boost::bind(callback, 3));
  loop.loop();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

  Thread thread(threadFunc);
  thread.start();

  loop.loop();
}
