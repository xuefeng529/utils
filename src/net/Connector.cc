#include "net/Connector.h"
#include "net/EventLoop.h"
#include "net/config.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <errno.h>

namespace net
{

void Connector::handleEvent(struct bufferevent* bev, short events, void* ctx)
{
	Connector* connector = static_cast<Connector*>(ctx);
	if (events & BEV_EVENT_CONNECTED)
	{
		connector->setState(kConnected);
		connector->newConnectionCallback_(bufferevent_getfd(connector->bev_));
	}
	else if (events & BEV_EVENT_ERROR)
	{
        LOG_ERROR << "Connector::handleEvent: " << base::strerror_tl(errno);
		connector->setState(kDisconnected);	
		int fd = bufferevent_getfd(connector->bev_);
		if (fd != -1)
		{
			EVUTIL_CLOSESOCKET(fd);
		}
		
		connector->connectingFailedCallback_();
	}
    else
    {
        LOG_ERROR << "Connector::handleEvent: unknow event," << events;
		int fd = bufferevent_getfd(connector->bev_);
		if (fd != -1)
		{
			EVUTIL_CLOSESOCKET(fd);
		}
    }

    connector->freeEvent();
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
    freeEvent();
}

void Connector::restart()
{
	loop_->runInLoop(boost::bind(&Connector::restartInLoop, this));
}

void Connector::restartInLoop()
{
	loop_->assertInLoopThread();
	setState(kDisconnected);
    connecting();
}

void Connector::connecting()
{
    LOG_INFO << "connecting " << serverAddr_.toIpPort();
    assert(bev_ == NULL);
    bev_ = bufferevent_socket_new(loop_->eventBase(), -1, 0);
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

void Connector::freeEvent()
{
    if (bev_ != NULL)
    {
        bufferevent_free(bev_);
        bev_ = NULL;
    }
}

} // namespace net
