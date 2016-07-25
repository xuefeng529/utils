#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/config.h"
#include "base/Logging.h"

#include <stdio.h>
#include <errno.h>

namespace net
{

void Acceptor::handleAccept(struct evconnlistener *listener,
							int fd,
							struct sockaddr *sa,
							int socklen,
							void *ctx)
{
	Acceptor* acceptor = static_cast<Acceptor*>(ctx);
	InetAddress addr(*reinterpret_cast<struct sockaddr_in*>(sa));
	if (acceptor->newConnectionCallback_)
	{
		acceptor->newConnectionCallback_(fd, addr);
	}
}

void Acceptor::handleAcceptError(struct evconnlistener *listener, void *ctx)
{
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	LOG_ERROR << "Got an error " << err << " " << evutil_socket_error_to_string(err) 
		<< " on the listener. Shutting down.\n";
	(void)base;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
	: loop_(loop),
	  listenAddr_(listenAddr)
{ 
}

Acceptor::~Acceptor()
{
	evconnlistener_free(listener_);
}

void Acceptor::listen()
{
	const struct sockaddr* sin = listenAddr_.getSockAddr();
	listener_ = evconnlistener_new_bind(loop_->eventBase(), handleAccept, this,
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC, -1, sin, sizeof(*sin));
	if (listener_ == NULL)
	{
		LOG_FATAL << "evconnlistener_new_bind of Acceptor::listen: "
			<< evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
	}

	evconnlistener_set_error_cb(listener_, handleAcceptError);
}

} // namespace net
