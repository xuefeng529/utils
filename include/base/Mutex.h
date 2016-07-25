#ifndef BASE_MUTEX_H
#define BASE_MUTEX_H

#include "base/CurrentThread.h"

#include <boost/noncopyable.hpp>

#include <assert.h>
#include <pthread.h>

namespace base
{

class MutexLock : boost::noncopyable
{
 public:
  MutexLock()
  {
	  pthread_mutex_init(&mutex_, NULL);
  }

  ~MutexLock()
  {
	  pthread_mutex_destroy(&mutex_);
  }

  void lock()
  {
	  pthread_mutex_lock(&mutex_);
  }

  void unlock()
  {
	  pthread_mutex_unlock(&mutex_);
  }

  pthread_mutex_t* getPthreadMutex() /* non-const */
  {
    return &mutex_;
  }

 private:
  friend class Condition;

  pthread_mutex_t mutex_;
};

class MutexLockGuard : boost::noncopyable
{
 public:
  explicit MutexLockGuard(MutexLock& mutex)
    : mutex_(mutex)
  {
    mutex_.lock();
  }

  ~MutexLockGuard()
  {
    mutex_.unlock();
  }

 private:
  MutexLock& mutex_;
};

}

#endif // BASE_MUTEX_H
