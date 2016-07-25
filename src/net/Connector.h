#ifndef NET_CONNECTOR_H
#define NET_CONNECTOR_H

#include "net/InetAddress.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

struct bufferevent;

namespace net
{

class EventLoop;

class Connector : boost::noncopyable,
				  public boost::enable_shared_from_this <Connector>
{
public:
	typedef boost::function<void(int sockfd)> NewConnectionCallback;
	typedef boost::function<void()> ConnectingFailedCallback;

	Connector(EventLoop* loop, const InetAddress& serverAddr);
	~Connector();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{ newConnectionCallback_ = cb; }

	void setConnectingFailedCallback(const ConnectingFailedCallback& cb)
	{ connectingFailedCallback_ = cb; }

	const InetAddress& serverAddress() const { return serverAddr_; }

	void start();
	void restart();
	
private:
	static void handleEvent(struct bufferevent *bev, short events, void *ctx);

	enum States { kDisconnected, kConnecting, kConnected };
	void setState(States s) { state_ = s; }
	void startInLoop();
	void restartInLoop();
	void connecting();

	EventLoop* loop_;
	const InetAddress serverAddr_;
	NewConnectionCallback newConnectionCallback_;
	ConnectingFailedCallback connectingFailedCallback_;
	States state_;
	struct bufferevent* bev_;
};

} // namespace net

#endif // NET_CONNECTOR_H
