#ifndef NET_EVENTLOOP_H
#define NET_EVENTLOOP_H

#include "base/Mutex.h"
#include "base/Timestamp.h"
#include "base/Atomic.h"
#include "net/Timer.h"

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

struct event_base;
struct bufferevent;

namespace net
{

class TimerQueue;

class EventLoop : boost::noncopyable
{
public:
	typedef boost::function<void()> Functor;

	EventLoop();
	~EventLoop();

	void loop();
	void quit();

	bool isInLoopThread() const
	{ return threadId_ == base::CurrentThread::tid(); }

	void runInLoop(const Functor& cb);
	void queueInLoop(const Functor& cb);

	int64_t runAfter(int64_t delay, const TimerCallback& cb);
	int64_t runEvery(int64_t interval, const TimerCallback& cb);
	void cancel(int64_t timerId);

	void assertInLoopThread()
	{
		if (!isInLoopThread())
		{
			abortNotInLoopThread();
		}
	}

	void abortNotInLoopThread();
	struct event_base* eventBase()
	{ return base_; }

	static EventLoop* getEventLoopOfCurrentThread();

private:
	static void handleRead(struct bufferevent *bev, void *ctx);
	static void handleWrite(struct bufferevent *bev, void *ctx);
	static void handleEvent(struct bufferevent *bev, short events, void *ctx);

	void wakeup();
	void doPendingFunctors();
	void quitInLoop();

	struct event_base* base_;
	const pid_t threadId_;
	base::MutexLock mutex_;
	std::vector<Functor> pendingFunctors_;
	boost::scoped_ptr<TimerQueue> timerQueue_;
	struct bufferevent* wakeupPair_[2];
};

} // namespace net

#endif // NET_EVENTLOOP_H
