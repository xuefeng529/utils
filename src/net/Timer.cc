#include "net/Timer.h"
#include "net/config.h"
#include "net/TimerQueue.h"
#include "net/EventLoop.h"
#include "base/Logging.h"

#include <errno.h>

namespace net
{

base::AtomicInt64 Timer::g_numCreated_;

void Timer::handleTimeout(int fd, short event, void* ctx)
{
    assert(ctx != NULL);
    Timer* timer = static_cast<Timer*>(ctx);
    if (timer->callback_)
    {
        timer->callback_();
        if (timer->repeat_)
        {
            timer->restart();
        }
        else
        {
            timer->timerQueue_->removeTimerInLoop(timer->sequence());
        }
    }
}

Timer::Timer(TimerQueue* timerQueue, const TimerCallback& cb, time_t interval, bool repeat)
    : timerQueue_(timerQueue),
      sequence_(g_numCreated_.incrementAndGet()),
      callback_(cb),
	  interval_(interval),
	  repeat_(repeat),
      timeoutEvent_(evtimer_new(timerQueue->getLoop()->eventBase(), handleTimeout, this))
{
    if (timeoutEvent_ == NULL)
    {
        LOG_FATAL << "evtimer_new of Timer::ctor: " << base::strerror_tl(errno);
    }
}

Timer::~Timer()
{
    assert(timeoutEvent_ != NULL);
    event_free(timeoutEvent_);
}

void Timer::restart()
{
    assert(timeoutEvent_ != NULL);
    struct timeval tv;
    bzero(&tv, sizeof(tv));
    tv.tv_sec = interval_;
    if (evtimer_add(timeoutEvent_, &tv) == -1)
    {
        LOG_ERROR << "evtimer_add of Timer::restart: " << base::strerror_tl(errno);
    }
}

} // namespace net
