#ifndef BASE_RW_LOCK_H
#define BASE_RW_LOCK_H

#include <boost/noncopyable.hpp>

#include <pthread.h>

#define CHECK_RET(ret) ({ __typeof__ (ret) errnum = (ret);     \
                          assert(errnum == 0); (void) errnum;})

namespace base
{

class RWLock : boost::noncopyable
{
public:
	RWLock()
	{
		CHECK_RET(pthread_rwlock_init(&rwlock_, NULL));
	}

	~RWLock()
	{
		CHECK_RET(pthread_rwlock_destroy(&rwlock_));
	}

	void rdlock()
	{
		CHECK_RET(pthread_rwlock_rdlock(&rwlock_));
	}

	void wrlock()
	{
		CHECK_RET(pthread_rwlock_wrlock(&rwlock_));
	}

	void unlock()
	{
		CHECK_RET(pthread_rwlock_unlock(&rwlock_));
	}

private:
	pthread_rwlock_t rwlock_;
};

class RLockGuard : boost::noncopyable
{
public:
	RLockGuard(RWLock* rwlock)
		: rwlock_(rwlock)
	{
		rwlock_->rdlock();
	}

	~RLockGuard()
	{
		rwlock_->unlock();
	}

private:
	RWLock* rwlock_;
};

class WLockGuard : boost::noncopyable
{
public:
	WLockGuard(RWLock* rwlock)
		: rwlock_(rwlock)
	{
		rwlock_->wrlock();
	}

	~WLockGuard()
	{
		rwlock_->unlock();
	}

private:
	RWLock* rwlock_;
};

} // namespace base

#endif // BASE_RW_LOCK_H
