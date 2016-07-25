#ifndef BASE_ASYNCLOGGING_H
#define BASE_ASYNCLOGGING_H

#include "base/BlockingQueue.h"
#include "base/BoundedBlockingQueue.h"
#include "base/CountDownLatch.h"
#include "base/Mutex.h"
#include "base/Thread.h"
#include "base/LogStream.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace base
{

class AsyncLogging : boost::noncopyable
{
 public:

  AsyncLogging(const std::string& dir,
			   const std::string& basename,
               size_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start()
  {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop()
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:

  // declare but not define, prevent compiler-synthesized functions
  AsyncLogging(const AsyncLogging&);  // ptr_container
  void operator=(const AsyncLogging&);  // ptr_container

  void threadFunc();

  typedef base::detail::FixedBuffer<base::detail::kLargeBuffer> Buffer;
  typedef boost::ptr_vector<Buffer> BufferVector;
  typedef BufferVector::auto_type BufferPtr;

  const int flushInterval_;
  bool running_;
  std::string dir_;
  std::string basename_;
  size_t rollSize_;
  Thread thread_;
  CountDownLatch latch_;
  MutexLock mutex_;
  Condition cond_;
  BufferPtr currentBuffer_;
  BufferPtr nextBuffer_;
  BufferVector buffers_;
};

}
#endif  // BASE_ASYNCLOGGING_H
