#ifndef BASE_MUTEX_H
#define BASE_MUTEX_H

#include "base/CurrentThread.h"

#include <boost/noncopyable.hpp>

#include <assert.h>
#include <pthread.h>

#define CHECK_RET(ret) ({ __typeof__ (ret) errnum = (ret);     \
                          assert(errnum == 0); (void) errnum;})

namespace base
{

class MutexLock : boost::noncopyable
{
public:
	MutexLock()
	{
		CHECK_RET(pthread_mutex_init(&mutex_, NULL));
	}

	~MutexLock()
	{
		CHECK_RET(pthread_mutex_destroy(&mutex_));
	}

	void lock()
	{
		CHECK_RET(pthread_mutex_lock(&mutex_));
	}

	void unlock()
	{
		CHECK_RET(pthread_mutex_unlock(&mutex_));
	}

	pthread_mutex_t* getPthreadMutex()
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

} // namespace base

#endif // BASE_MUTEX_H
