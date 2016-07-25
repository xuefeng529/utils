#include "base/Thread.h"
#include "base/CurrentThread.h"

#include <string>
#include <boost/bind.hpp>
#include <stdio.h>
#include <unistd.h>

void mysleep(int seconds)
{
  timespec t = { seconds, 0 };
  nanosleep(&t, NULL);
}

void threadFunc()
{
  printf("tid=%d\n", base::CurrentThread::tid());
}

void threadFunc2(int x)
{
  printf("tid=%d, x=%d\n", base::CurrentThread::tid(), x);
}

void threadFunc3()
{
  printf("tid=%d\n", base::CurrentThread::tid());
  mysleep(1);
}

class Foo
{
 public:
  explicit Foo(double x)
    : x_(x)
  {
  }

  void memberFunc()
  {
    printf("tid=%d, Foo::x_=%f\n", base::CurrentThread::tid(), x_);
  }

  void memberFunc2(const std::string& text)
  {
    printf("tid=%d, Foo::x_=%f, text=%s\n", base::CurrentThread::tid(), x_, text.c_str());
  }

 private:
  double x_;
};

int main()
{
  printf("pid=%d, tid=%d\n", ::getpid(), base::CurrentThread::tid());

  base::Thread t1(threadFunc);
  t1.start();
  t1.join();

  base::Thread t2(boost::bind(threadFunc2, 42),
                   "thread for free function with argument");
  t2.start();
  t2.join();

  Foo foo(99.99);
  base::Thread t3(boost::bind(&Foo::memberFunc, &foo),
                   "thread for member function without argument");
  t3.start();
  t3.join();

  base::Thread t4(boost::bind(&Foo::memberFunc2, boost::ref(foo), std::string("EastMoney")));
  t4.start();
  t4.join();

  {
    base::Thread t5(threadFunc3);
    t5.start();
  }
  mysleep(2);
  {
    base::Thread t6(threadFunc3);
    t6.start();
    mysleep(2);
  }
  sleep(2);
  printf("number of created threads %d\n", base::Thread::numCreated());
}
