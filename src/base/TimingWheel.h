#ifndef BASE_TIMINGWHEEL_H
#define BASE_TIMINGWHEEL_H

#include "base/LockFree.h"
#include "base/Atomic.h"

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <deque>
#include <vector>

namespace base
{

class Thread;

class TimingWheel : boost::noncopyable
{
public:
	typedef boost::function<void()> TimerTask;

	class Timeout
	{
	public:
		Timeout() {}
		Timeout(const TimerTask& task, uint64_t delay) : task_(task), delay_(delay) {}
		Timeout(const Timeout& other) : task_(other.task_), delay_(other.delay_) {}
		uint64_t delay() const { return delay_; }
		operator bool() const { return task_; }
		void operator ()() const
		{
			if (task_)
			{
				task_();
			}
		}

		Timeout& operator=(const Timeout& other)
		{
			if (&other != this)
			{
				task_ = other.task_;
				delay_ = other.delay_;
			}

			return *this;
		}

	private:
		TimerTask task_;
		uint64_t delay_;
	};

	/// @tickDuration: the duration between tick (millisecond)
	/// @ticksPerWheel: the size of the wheel
	TimingWheel(uint64_t tickDuration, uint64_t ticksPerWheel);
	~TimingWheel();

	void start();
	void stop();

	bool addTimeout(const TimerTask& task, uint64_t delay);

private:
	void timerFunc();
	void transferTimeoutsToBuckets();
	void expireTimeouts();

	base::AtomicInt32 started_;
	const uint64_t tickDuration_;
	const uint64_t ticksPerWheel_;
	uint64_t tick_;

	typedef base::lockfree::ArrayLockFreeQueue<Timeout, 100000> TimeoutQueue;
	boost::scoped_ptr<TimeoutQueue> timeouts_;

	typedef std::deque<TimerTask> TimerTaskDeque;
	std::vector<TimerTaskDeque> wheel_;
	boost::scoped_ptr<Thread> thread_;
};

} // namespace base

#endif // BASE_TIMINGWHEEL_H
