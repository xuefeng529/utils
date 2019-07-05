#include "base/TimingWheel.h"
#include "base/Thread.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

namespace base
{

TimingWheel::TimingWheel(uint64_t tickDuration, uint64_t ticksPerWheel)
	: tickDuration_(tickDuration),
	  ticksPerWheel_(ticksPerWheel),
	  tick_(0),
	  timeouts_(new TimeoutBufferQueue()),
	  wheel_(ticksPerWheel),
	  thread_(new base::Thread(boost::bind(&TimingWheel::timerFunc, this)))
{
}

TimingWheel::~TimingWheel()
{
	if (started_.get() == 1)
	{
		stop();
	}
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

TimingWheel::TimeoutPtr TimingWheel::addTimeout(const TimerTask& task, uint64_t delay, const std::string& name)
{
	if (delay % tickDuration_ || delay / tickDuration_ > ticksPerWheel_)
	{
		LOG_ERROR << "adding timeout errors, name: " << name << ", delay: " << delay;
		return TimeoutPtr();
	}

	TimeoutPtr timeout(new Timeout(task, delay, name));
	if (!timeouts_->push(timeout))
	{
		LOG_WARN << "timeout buffer queue full, name: " << name << ", delay: " << delay;
		return TimeoutPtr();
	}

	return timeout;
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
	const uint64_t nextTickDeadline = Timestamp::now().microSecondsSinceEpoch() / 1000 + tickDuration_;
	for (int i = 0; i < 100000; ++i)
	{
		TimeoutPtr timeout;
		if (!timeouts_->pop(timeout))
		{
			break;
		}

		if (timeout->deadline() < nextTickDeadline)
		{
			LOG_WARN << "the timeout in buffer queue has expired, name: "
				<< timeout->name() << ", delay: " << timeout->delay();
			timeout->expire();
			continue;
		}

		uint64_t idx = (tick_ + timeout->delay() / tickDuration_) % ticksPerWheel_;
		wheel_[idx].push_back(timeout);
	}
}

void TimingWheel::expireTimeouts()
{
	tick_ = (tick_ + 1) % ticksPerWheel_;
	TimeoutDeque& buckets = wheel_[tick_];
	TimeoutDeque::const_iterator it = buckets.begin();
	for (; it != buckets.end(); ++it)
	{
		(*it)->expire();
	}

	buckets.clear();
}

} // namespace base
