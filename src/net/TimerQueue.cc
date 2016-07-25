#include "net/Timer.h"
#include "net/TimerQueue.h"
#include "net/EventLoop.h"

#include <boost/bind.hpp>

namespace net
{

TimerQueue::TimerQueue(EventLoop* loop)
	: loop_(loop)
{
}

TimerQueue::~TimerQueue()
{
	std::map<int64_t, Timer*>::iterator it = timers_.begin();
	for ( ; it != timers_.end(); ++it)
	{
		it->second->stop();
		delete it->second;
	}
}

int64_t TimerQueue::addTimer(const TimerCallback& cb, uint64_t interval, bool repeat)
{
	Timer* timer = new Timer(loop_, cb, interval, repeat);
	loop_->runInLoop(boost::bind(&TimerQueue::addTimerInLoop, this, timer));
	return timer->sequence();
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
	loop_->assertInLoopThread();
	timers_.insert(std::make_pair(timer->sequence(), timer));
	timer->start();
}

void TimerQueue::cancel(int64_t timerId)
{
	loop_->runInLoop(boost::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::cancelInLoop(int64_t timerId)
{
	loop_->assertInLoopThread();
	std::map<int64_t, Timer*>::iterator it = timers_.find(timerId);
	if (it != timers_.end())
	{
		it->second->stop();
		timers_.erase(timerId);
	}
}

} // namespace net
