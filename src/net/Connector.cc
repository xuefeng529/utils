#include "net/Connector.h"
#include "net/EventLoop.h"
#include "net/config.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <errno.h>

namespace net
{

void Connector::handleEvent(struct bufferevent *bev, short events, void *ctx)
{
	Connector* connector = static_cast<Connector*>(ctx);
	if (events & BEV_EVENT_CONNECTED)
	{
		connector->setState(kConnected);
		connector->newConnectionCallback_(bufferevent_getfd(connector->bev_));
	}
	else if (events & BEV_EVENT_ERROR)
	{
		connector->setState(kDisconnected);
		LOG_ERROR << "Connector::handleEvent: " << base::strerror_tl(errno);
		connector->connectingFailedCallback_();
	}
}

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
	: loop_(loop),
	  serverAddr_(serverAddr),
	  state_(kDisconnected),
	  bev_(NULL)
{

}

Connector::~Connector()
{
	bufferevent_free(bev_);
}

void Connector::start()
{
	loop_->runInLoop(boost::bind(&Connector::startInLoop, this));
}

void Connector::restart()
{
	loop_->runInLoop(boost::bind(&Connector::restartInLoop, this));
}

void Connector::restartInLoop()
{
	loop_->assertInLoopThread();
	setState(kDisconnected);
	startInLoop();
}

void Connector::startInLoop()
{
	loop_->assertInLoopThread();
	if (state_ == kDisconnected)
	{
		connecting();
	}
}

void Connector::connecting()
{
	if (bev_ != NULL)
	{
		bufferevent_free(bev_);
	}

	bev_ = bufferevent_socket_new(loop_->eventBase(), -1, BEV_OPT_CLOSE_ON_FREE);
	if (bev_ == NULL)
	{
		LOG_FATAL << "bufferevent_socket_new of Connector::connecting: "
			<< base::strerror_tl(errno);
	}

	bufferevent_setcb(bev_, NULL, NULL, handleEvent, this);
	const struct sockaddr* sin = serverAddr_.getSockAddr();
	if (bufferevent_socket_connect(bev_, const_cast<struct sockaddr*>(sin), sizeof(struct sockaddr_in)) == -1)
	{
		LOG_FATAL << "bufferevent_socket_connect of Connector::connecting: "
			<< base::strerror_tl(errno);
	}
	
	setState(kConnecting);
}

} // namespace net
