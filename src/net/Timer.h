#ifndef NET_TIMER_H
#define NET_TIMER_H

#include "base/Atomic.h"
#include "base/Timestamp.h"

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

struct event_base;
struct event;

namespace net
{

class EventLoop;

typedef boost::function<void()> TimerCallback;

class Timer : boost::noncopyable
{
public:
	Timer(EventLoop* loop, const TimerCallback& cb, time_t interval, bool repeat);
	~Timer();

	void start();
	void stop();
	int64_t sequence() const { return sequence_; }

	static int64_t numCreated() { return g_numCreated_.get(); }

private:
	static void handleTimeout(int fd, short event, void *arg);

	void activate();

	static base::AtomicInt64 g_numCreated_;

	EventLoop* loop_;
	const TimerCallback callback_;
	const time_t interval_;
	const bool repeat_;
	bool avtive_;
	const int64_t sequence_;
	struct event* timeout_;
};

} // namespace net

#endif // NET_TIMER_H
