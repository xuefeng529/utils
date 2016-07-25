#ifndef BASE_CONDITION_H
#define BASE_CONDITION_H

#include "base/Mutex.h"

#include <boost/noncopyable.hpp>

#include <pthread.h>

namespace base
{

class Condition : boost::noncopyable
{
 public:
  explicit Condition(MutexLock& mutex)
    : mutex_(mutex)
  {
	  pthread_cond_init(&pcond_, NULL);
  }

  ~Condition()
  {
	  pthread_cond_destroy(&pcond_);
  }

  void wait()
  {
	  pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
  }

  // returns true if time out, false otherwise.
  bool waitForSeconds(int seconds);

  void notify()
  {
	  pthread_cond_signal(&pcond_);
  }

  void notifyAll()
  {
	  pthread_cond_broadcast(&pcond_);
  }

 private:
  MutexLock& mutex_;
  pthread_cond_t pcond_;
};

}

#endif // BASE_CONDITION_H
