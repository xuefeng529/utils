#ifndef BASE_TIMINGWHEEL_H
#define BASE_TIMINGWHEEL_H

#include "base/LockFree.h"
#include "base/Atomic.h"
#include "base/Timestamp.h"

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
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
		Timeout(const TimerTask& task, uint64_t delay, const std::string& name)
			: task_(task), 
			  delay_(delay), 
			  name_(name),
			  deadline_(Timestamp::now().microSecondsSinceEpoch() / 1000 + delay)
		{
		}
		
		const std::string& name() const { return name_; }	
		uint64_t delay() const { return delay_; }
		void cancell() { cancelled_.getAndSet(1); }
		
	private:
		friend class TimingWheel;

		uint64_t deadline() const { return deadline_; }
		void expire() const
		{
			if (cancelled_.get() == 0 && task_)
			{
				task_();
			}
		}

		const TimerTask task_;
		const uint64_t delay_;
		const std::string name_;
		const uint64_t deadline_;
		mutable base::AtomicInt32 cancelled_;
	};

	typedef boost::shared_ptr<Timeout> TimeoutPtr;

	/// @tickDuration: the duration between tick (millisecond)
	/// @ticksPerWheel: the size of the wheel
	TimingWheel(uint64_t tickDuration, uint64_t ticksPerWheel);
	~TimingWheel();

	void start();
	void stop();

	TimeoutPtr addTimeout(const TimerTask& task, uint64_t delay, const std::string& name = std::string());

private:
	void timerFunc();
	void transferTimeoutsToBuckets();
	void expireTimeouts();

	base::AtomicInt32 started_;
	const uint64_t tickDuration_;
	const uint64_t ticksPerWheel_;
	uint64_t tick_;

	typedef base::lockfree::ArrayLockFreeQueue<TimeoutPtr, 100000> TimeoutBufferQueue;
	boost::scoped_ptr<TimeoutBufferQueue> timeouts_;

	typedef std::deque<TimeoutPtr> TimeoutDeque;
	std::vector<TimeoutDeque> wheel_;
	boost::scoped_ptr<Thread> thread_;
};

} // namespace base

#endif // BASE_TIMINGWHEEL_H
