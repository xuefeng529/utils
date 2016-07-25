#include "base/ThreadPool.h"
#include "base/CountDownLatch.h"
#include "base/CurrentThread.h"
#include "base/StringUtil.h"

#include <boost/bind.hpp>
#include <stdio.h>
#include <iostream>

void print()
{
  printf("tid=%d\n", base::CurrentThread::tid());
}

void printString(const std::string& str)
{
  std::cout << str << std::endl;
  //usleep(100*1000);
}

void test(int maxSize)
{
  std::cout << "Test ThreadPool with max queue size = " << maxSize << std::endl;
  base::ThreadPool pool("MainThreadPool");
  pool.setMaxQueueSize(maxSize);
  pool.start(5);

  std::cout << "Adding" << std::endl;
  pool.run(print);
  pool.run(print);
  for (int i = 0; i < 100; ++i)
  {
    char buf[32];
    snprintf(buf, sizeof buf, "task %d", i);
    pool.run(boost::bind(printString, std::string(buf)));
  }
  std::cout << "Done" << std::endl;

  base::CountDownLatch latch(1);
  pool.run(boost::bind(&base::CountDownLatch::countDown, &latch));
  latch.wait();
  pool.stop();
}

base::ThreadPool pool("ThreadPool");

int main()
{
	pool.setMaxQueueSize(10000);
	pool.start(1);

	for (size_t i = 0; i < 10000; i++)
	{
		pool.run(boost::bind(&printString, base::StringUtil::uint64ToStr(i)));
	}

	base::CountDownLatch latch(1);
	pool.run(boost::bind(&base::CountDownLatch::countDown, &latch));
	latch.wait();
	pool.stop();
}
