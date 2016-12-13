#include "net/Timer.h"
#include "net/EventLoop.h"
#include "net/config.h"
#include "base/Logging.h"

#include <errno.h>

namespace net
{

base::AtomicInt64 Timer::g_numCreated_;

void Timer::handleTimeout(int fd, short event, void *ctx)
{
	Timer* timer = static_cast<Timer*>(ctx);
	if (timer->callback_)
	{
		timer->callback_();
		if (timer->avtive_ && timer->repeat_)
		{
			timer->activate();
		}
	}
}

Timer::Timer(EventLoop* loop, const TimerCallback& cb, time_t interval, bool repeat)
	: loop_(loop),
	  callback_(cb),
	  interval_(interval),
	  repeat_(repeat),
	  avtive_(false),
	  sequence_(g_numCreated_.incrementAndGet())
{
}

Timer::~Timer()
{
	event_free(timeout_);
}

void Timer::start()
{
	if (!avtive_)
	{
		avtive_ = true;
		timeout_ = evtimer_new(loop_->eventBase(), handleTimeout, this);
		if (timeout_ == NULL)
		{
			LOG_FATAL << "evtimer_new of Timer::start: "
				<< base::strerror_tl(errno);
		}

		activate();
	}
}

void Timer::stop()
{
	if (avtive_)
	{
		avtive_ = false;
		evtimer_del(timeout_);
	}
}

void Timer::activate()
{
	struct timeval tv;
	bzero(&tv, sizeof(tv));
	tv.tv_sec = interval_;
	if (evtimer_add(timeout_, &tv) == -1)
	{
		LOG_ERROR << "evtimer_add of Timer::activate: "
			<< base::strerror_tl(errno);
	}
}

} // namespace net
