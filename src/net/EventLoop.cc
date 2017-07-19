#include "net/EventLoop.h"
#include "net/config.h"
#include "net/TimerQueue.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

namespace net
{

__thread EventLoop* t_loopInThisThread = 0;

#pragma GCC diagnostic ignored "-Wold-style-cast"
class InitialConfig
{
public:
	InitialConfig()
	{
		::signal(SIGPIPE, SIG_IGN);
		evthread_use_pthreads();
	}
};
#pragma GCC diagnostic error "-Wold-style-cast"

InitialConfig initialConfig;

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
	return t_loopInThisThread;
}

void EventLoop::handleRead(struct bufferevent *bev, void *ctx)
{
	EventLoop* loop = static_cast<EventLoop*>(ctx);
	struct evbuffer* input = bufferevent_get_input(bev);
	if (input == NULL)
	{
		LOG_ERROR << "bufferevent_get_input of EventLoop::wakup: "
			<< base::strerror_tl(errno);
		return;
	}

	evbuffer_drain(input, evbuffer_get_length(input));
	loop->doPendingFunctors();
}

void EventLoop::handleWrite(struct bufferevent *bev, void *ctx)
{
	LOG_DEBUG << "EventLoop::handleWrite";
	struct evbuffer* evbuf = bufferevent_get_output(bev);
	(void)evbuf;
	assert(evbuffer_get_length(evbuf) == 0);
}

void EventLoop::handleEvent(struct bufferevent* bev, short events, void* ctx)
{
	EventLoop* loop = static_cast<EventLoop*>(ctx);
	(void)loop;
	if (events & BEV_EVENT_EOF)
	{
		LOG_DEBUG << "EventLoop::handleEvent: BEV_EVENT_EOF";
	}
	else if (events & BEV_EVENT_ERROR)
	{
		LOG_DEBUG << "EventLoop::handleEvent: BEV_EVENT_ERROR";
	}
}

EventLoop::EventLoop()
	: base_(event_base_new()),
	  threadId_(base::CurrentThread::tid()),
	  timerQueue_(new TimerQueue(this))
{
	LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
	if (base_ == NULL)
	{
		LOG_FATAL << "event_base_new of EventLoop::EventLoop(): "
			<< base::strerror_tl(errno);
	}

    LOG_INFO << "event method: " << event_base_get_method(base_);
	
	evutil_socket_t pair[2];
	evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
	evutil_make_socket_nonblocking(pair[0]);
	evutil_make_socket_nonblocking(pair[1]);
	wakeupPair_[0] = bufferevent_socket_new(base_, pair[0], BEV_OPT_CLOSE_ON_FREE);
	wakeupPair_[1] = bufferevent_socket_new(base_, pair[1], BEV_OPT_CLOSE_ON_FREE|BEV_OPT_THREADSAFE);
	if (wakeupPair_[0] == NULL || wakeupPair_[1] == NULL)
	{
		LOG_FATAL << "bufferevent_socket_new of EventLoop::EventLoop(): "
			<< base::strerror_tl(errno);
	}
	
	bufferevent_setcb(wakeupPair_[0], handleRead, NULL, handleEvent, this);
	bufferevent_setcb(wakeupPair_[1], NULL, NULL, handleEvent, this);
	bufferevent_enable(wakeupPair_[0], EV_READ);
	if (t_loopInThisThread)
	{
		LOG_FATAL << "Another EventLoop " << t_loopInThisThread
			<< " exists in this thread " << threadId_;
	}
	else
	{
		t_loopInThisThread = this;
	}
}

EventLoop::~EventLoop()
{
	LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
		<< " destructs in thread " << base::CurrentThread::tid();
	bufferevent_disable(wakeupPair_[0], EV_READ | EV_WRITE);
	bufferevent_disable(wakeupPair_[1], EV_READ | EV_WRITE);
	bufferevent_free(wakeupPair_[0]);
	bufferevent_free(wakeupPair_[1]);
	event_base_free(base_);
	t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
	assertInLoopThread();
	if (event_base_dispatch(base_) == -1)
	{
		LOG_FATAL << "event_base_dispatch of EventLoop::loop: " 
			<< base::strerror_tl(errno);
	}
}

void EventLoop::quit()
{
	if (isInLoopThread())
	{
		quitInLoop();
	}
	else
	{
		runInLoop(boost::bind(&EventLoop::quitInLoop, this));
	}
}

void EventLoop::quitInLoop()
{
	assertInLoopThread();
	struct timeval delay = { 2, 0 };
	event_base_loopexit(base_, &delay);
}

void EventLoop::abortNotInLoopThread()
{
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
		<< " was created in threadId_ = " << threadId_
		<< ", current thread id = " << base::CurrentThread::tid();
}

void EventLoop::runInLoop(const Functor& cb)
{
	if (isInLoopThread())
	{
		cb();
	}
	else
	{
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(const Functor& cb)
{
	{
		base::MutexLockGuard lock(mutex_);
		pendingFunctors_.push_back(cb);
	}
	
	wakeup();
}

int64_t EventLoop::runAfter(int64_t delay, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, delay, false);
}

int64_t EventLoop::runEvery(int64_t interval, const TimerCallback& cb)
{
	return timerQueue_->addTimer(cb, interval, true);
}

void EventLoop::cancel(int64_t timerId)
{
	timerQueue_->cancel(timerId);
}

void EventLoop::wakeup()
{
	struct evbuffer* output = bufferevent_get_output(wakeupPair_[1]);
	if (output == NULL)
	{
		LOG_FATAL << "bufferevent_get_output of EventLoop::wakeup: "
			<< base::strerror_tl(errno);
	}

	uint8_t one = 1;
	if (evbuffer_add(output, &one, sizeof(one)) == -1)
	{
		LOG_FATAL << "evbuffer_add of EventLoop::wakup: "
			<< base::strerror_tl(errno);
	}
}

void EventLoop::doPendingFunctors()
{
	std::vector<Functor> functors;

	{
		base::MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);
	}

	for (size_t i = 0; i < functors.size(); ++i)
	{
		functors[i]();
	}
}

} // namespace net
