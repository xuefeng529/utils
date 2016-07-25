#ifndef NET_ACCEPTOR_H
#define NET_ACCEPTOR_H

#include "net/InetAddress.h"

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>

struct evconnlistener;

namespace net
{

class EventLoop;

class Acceptor : boost::noncopyable
{
public:
	typedef boost::function <void(int sockfd, const InetAddress&)> NewConnectionCallback;

	Acceptor(EventLoop* loop, const InetAddress& listenAddr);
	~Acceptor();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{ newConnectionCallback_ = cb; }

	void listen();

private:
	static void handleAccept(struct evconnlistener *listener, 
							 int fd,
							 struct sockaddr *sa, 
							 int socklen,
							 void *ctx);

	static void handleAcceptError(struct evconnlistener *listener, void *ctx);

	EventLoop* loop_;
	const InetAddress listenAddr_;
	struct evconnlistener* listener_;
	NewConnectionCallback newConnectionCallback_;
};

} // namespace net

#endif // NET_ACCEPTOR_H
