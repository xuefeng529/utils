#ifndef BASE_SPINLOCK_H
#define BASE_SPINLOCK_H

#include <boost/noncopyable.hpp>

namespace base
{

class SpinLock : boost::noncopyable
{
public:
    SpinLock() : lock_(0) {}

    void lock()
    {
        while (__sync_lock_test_and_set(&lock_, 1)) {}
    }

    void unlock()
    {
        __sync_lock_release(&lock_);
    }

private:
    volatile int lock_;
};

class SpinLockGuard : boost::noncopyable
{
public:
    SpinLockGuard(SpinLock* lock)
        : lock_(lock)
    {
        lock_->lock();
    }

    ~SpinLockGuard()
    {
        lock_->unlock();
    }

private:
    SpinLock* lock_;
};

} // namespace base

#endif // BASE_SPINLOCK_H
