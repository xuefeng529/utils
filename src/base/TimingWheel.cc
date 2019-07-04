#include "base/TimingWheel.h"
#include "base/Thread.h"

#include <boost/bind.hpp>

namespace base
{

TimingWheel::TimingWheel(uint64_t tickDuration, uint64_t ticksPerWheel)
	: tickDuration_(tickDuration),
	  ticksPerWheel_(ticksPerWheel),
	  tick_(0),
	  timeouts_(new TimeoutQueue()),
	  wheel_(ticksPerWheel),
	  thread_(new base::Thread(boost::bind(&TimingWheel::timerFunc, this)))
{
}

TimingWheel::~TimingWheel()
{
}

void TimingWheel::start()
{
	if (started_.getAndSet(1) == 0)
	{
		thread_->start();
	}
}

void TimingWheel::stop()
{
	if (started_.getAndSet(0) == 1)
	{
		thread_->join();
	}
}

bool TimingWheel::addTimeout(const TimerTask& task, uint64_t delay)
{
	if (delay / tickDuration_ > ticksPerWheel_)
	{
		return false;
	}

	return timeouts_->push(Timeout(task, delay));
}

void TimingWheel::timerFunc()
{
	while (started_.get() == 1)
	{
		transferTimeoutsToBuckets();
		usleep(tickDuration_ * 1000);
		expireTimeouts();
	}
}

void TimingWheel::transferTimeoutsToBuckets()
{
	for (int i = 0; i < 100000; ++i)
	{
		Timeout timeout;
		if (!timeouts_->pop(timeout))
		{
			break;
		}

		uint64_t idx = (tick_ + timeout.delay() / tickDuration_) % ticksPerWheel_;
		wheel_[idx].push_back(timeout);
	}
}

void TimingWheel::expireTimeouts()
{
	tick_ = (tick_ + 1) % ticksPerWheel_;
	TimerTaskDeque& buckets = wheel_[tick_];
	TimerTaskDeque::const_iterator it = buckets.begin();
	for (; it != buckets.end(); ++it)
	{
		const TimerTask& task = *it;
		if (task)
		{
			task();
		}
	}

	buckets.clear();
}

} // namespace base
