#ifndef NET_EVENTLOOP_H
#define NET_EVENTLOOP_H

#include "base/LockFree.h"
#include "base/Mutex.h"
#include "base/Timestamp.h"
#include "base/Atomic.h"
#include "net/config.h"
#include "net/Timer.h"

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

#include <linux/version.h>

#ifdef EVENT_LOOP_QUEUE_SIZE
const uint32_t kEventLoopQueueSize = EVENT_LOOP_QUEUE_SIZE;
#else
const uint32_t kEventLoopQueueSize = 10000000;
#endif

struct event;
struct event_base;
struct bufferevent;

namespace net
{

class TimerQueue;

class EventLoop : boost::noncopyable
{
public:
	typedef boost::function<void()> Functor;

	EventLoop(const std::string& name = std::string());
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
    const std::string name_;
	const pid_t threadId_;
	base::MutexLock mutex_;
	std::vector<Functor> pendingFunctors_;
    //typedef base::lockfree::ArrayLockFreeQueue<Functor, kEventLoopQueueSize> PendingFunctorQueue;
    //boost::scoped_ptr<PendingFunctorQueue> pendingFunctors_;
	boost::scoped_ptr<TimerQueue> timerQueue_;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    static void handleRead(evutil_socket_t fd, short events, void* ctx);
    int wakeupFd_;
    event* wakeupEvent_;
#else
    struct bufferevent* wakeupPair_[2];
#endif
};

} // namespace net

#endif // NET_EVENTLOOP_H
