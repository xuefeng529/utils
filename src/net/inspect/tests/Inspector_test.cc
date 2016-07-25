#include "net/inspect/Inspector.h"
#include "net/EventLoop.h"
#include "net/EventLoopThread.h"


int main()
{
  net::EventLoop loop;
  net::EventLoopThread t;
  net::Inspector ins(t.startLoop(), net::InetAddress(9876), "test");
  loop.loop();
}

