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

int64_t TimerQueue::addTimer(const TimerCallback& cb, uint64_t interval, bool repeat)
{
	boost::shared_ptr<Timer> timer(new Timer(this, cb, interval, repeat));
	loop_->runInLoop(boost::bind(&TimerQueue::addTimerInLoop, this, timer));
	return timer->sequence();
}

void TimerQueue::addTimerInLoop(const boost::shared_ptr<Timer>& timer)
{
	loop_->assertInLoopThread();
	timers_.insert(std::make_pair(timer->sequence(), timer));
	timer->restart();
}

void TimerQueue::cancel(int64_t timerId)
{
	loop_->runInLoop(boost::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::cancelInLoop(int64_t timerId)
{
	loop_->assertInLoopThread();
    boost::unordered_map<int64_t, boost::shared_ptr<Timer> >::const_iterator it = timers_.find(timerId);
	if (it != timers_.end() && it->second->repeat())
	{
        it->second->disableRepeat();
	}
}

void TimerQueue::removeTimerInLoop(int64_t timerId)
{
    loop_->assertInLoopThread();
    boost::unordered_map<int64_t, boost::shared_ptr<Timer> >::const_iterator it = timers_.find(timerId);
    if (it != timers_.end())
    {
        timers_.erase(timerId);
    }
}

} // namespace net
