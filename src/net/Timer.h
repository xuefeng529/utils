#ifndef NET_TIMER_H
#define NET_TIMER_H

#include "base/Atomic.h"

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

struct event;

namespace net
{

class TimerQueue;

typedef boost::function<void()> TimerCallback;

class Timer : boost::noncopyable
{
public:
    static int64_t numCreated() { return g_numCreated_.get(); }

    Timer(TimerQueue* timerQueue, const TimerCallback& cb, time_t interval, bool repeat);
	~Timer();

    void restart();
    int64_t sequence() const { return sequence_; }
    bool repeat() const { return repeat_; }
    void disableRepeat() { repeat_ = false; }
	
private:
	static void handleTimeout(int fd, short event, void* arg);

	static base::AtomicInt64 g_numCreated_;

    TimerQueue* timerQueue_;
    const int64_t sequence_;
	const TimerCallback callback_;
	const time_t interval_;
    bool repeat_;
	struct event* timeoutEvent_;
};

} // namespace net

#endif // NET_TIMER_H
