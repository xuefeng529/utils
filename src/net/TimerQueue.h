#ifndef NET_TIMERQUEUE_H
#define NET_TIMERQUEUE_H

#include "base/Mutex.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace net
{

class EventLoop;
class Timer;

class TimerQueue : boost::noncopyable
{
public:
	TimerQueue(EventLoop* loop);

	int64_t addTimer(const TimerCallback& cb, uint64_t interval, bool repeat);
	void cancel(int64_t timerId);
    EventLoop* getLoop() const { return loop_; }

private:
    friend class Timer;

    void addTimerInLoop(const boost::shared_ptr<Timer>& timer);
	void cancelInLoop(int64_t timerId);
    void removeTimerInLoop(int64_t timerId);

	EventLoop* loop_;
	boost::unordered_map<int64_t, boost::shared_ptr<Timer> > timers_;
};

} // namespace net

#endif // NET_TIMERQUEUE_H
