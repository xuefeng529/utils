#ifndef NET_EVENTLOOPTHREAD_H
#define NET_EVENTLOOPTHREAD_H

#include "base/Condition.h"
#include "base/Mutex.h"
#include "base/Thread.h"

#include <boost/noncopyable.hpp>

namespace net
{

class EventLoop;

class EventLoopThread : boost::noncopyable
{
public:
	typedef boost::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
					const std::string& name = std::string());
	~EventLoopThread();
	EventLoop* startLoop();

private:
	void threadFunc();

	EventLoop* loop_;
    const std::string name_;
	bool exiting_;
	base::Thread thread_;
	base::MutexLock mutex_;
	base::Condition cond_;
	ThreadInitCallback callback_;
};

} // namespace net

#endif // NET_EVENTLOOPTHREAD_H
