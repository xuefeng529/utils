#ifndef NET_EVENTLOOPTHREADPOOL_H
#define NET_EVENTLOOPTHREADPOOL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <vector>

namespace net
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : boost::noncopyable
{
public:
	typedef boost::function<void(EventLoop*)> ThreadInitCallback;

	EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg);
	~EventLoopThreadPool();
	void setThreadNum(int numThreads) { numThreads_ = numThreads; }
	void start(const ThreadInitCallback& cb = ThreadInitCallback());

	/// round-robin
	EventLoop* getNextLoop();

	/// with the same hash code, it will always return the same EventLoop
	EventLoop* getLoopForHash(size_t hashCode);

	std::vector<EventLoop*> getAllLoops();

	bool started() const
	{ return started_; }

	const std::string& name() const
	{ return name_; }

private:
	EventLoop* baseLoop_;
	const std::string name_;
	bool started_;
	int numThreads_;
	int next_;
	boost::ptr_vector<EventLoopThread> threads_;
	std::vector<EventLoop*> loops_;
};

} // namespace net

#endif // NET_EVENTLOOPTHREADPOOL_H
