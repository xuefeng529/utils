#ifndef NET_TIMERQUEUE_H
#define NET_TIMERQUEUE_H

#include "base/Mutex.h"

#include <boost/noncopyable.hpp>
#include <map>

namespace net
{

class EventLoop;
class Timer;

class TimerQueue : boost::noncopyable
{
public:
	TimerQueue(EventLoop* loop);
	~TimerQueue();

	int64_t addTimer(const TimerCallback& cb, uint64_t interval, bool repeat);
	void cancel(int64_t timerId);

private:
	void addTimerInLoop(Timer* timer);
	void cancelInLoop(int64_t timerId);

	EventLoop* loop_;
	std::map<int64_t, Timer*> timers_;
};

} // namespace net

#endif // NET_TIMERQUEUE_H
