#ifndef NET_TCPCLIENT_H
#define NET_TCPCLIENT_H

#include "base/Mutex.h"
#include "net/TcpConnection.h"

#include <boost/noncopyable.hpp>

namespace net
{

class EventLoop;
class Connector;

typedef boost::function<void()> ConnectingExpireCallback;
typedef boost::function<void(const TcpConnectionPtr&)> HearbeatCallback;

class TcpClient : boost::noncopyable
{
public:
	TcpClient(EventLoop* loop,
			  const InetAddress& serverAddr,
			  const std::string& name,
			  time_t connectingExpire,
			  time_t heartbeat);
	~TcpClient(); 

	void connect();
	void disconnect();

	EventLoop* getLoop() const { return loop_; }
	bool retry() const;
	void enableRetry() { retry_ = true; }

	const std::string& name() const
	{ return name_; }

	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }

	void setConnectingExpireCallback(const ConnectingExpireCallback& cb)
	{ connectingExpireCallback_ = cb; }

	void setHearbeatCallback(const HearbeatCallback& cb)
	{ hearbeatCallback_ = cb; }

private:
	void newConnection(int sockfd);
	void connectingFailed();
	void removeConnection(const TcpConnectionPtr& conn);
	void connectInLoop();
	void disconnectInLoop();
	void handleHearbeat();

	static const time_t kMaxRetryDelayS = 60;
	static const time_t kInitRetryDelayS = 2;

	EventLoop* loop_;
	typedef boost::shared_ptr<Connector> ConnectorPtr;
	ConnectorPtr connector_;
	const std::string name_;
	const time_t connectingExpire_;
	const time_t heartbeat_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	ConnectingExpireCallback connectingExpireCallback_;
	HearbeatCallback hearbeatCallback_;
	bool connect_;
	int nextConnId_;
	TcpConnectionPtr connection_;
	time_t retryDelayS_;
	bool retry_;
};

} // namespace net

#endif // NET_TCPCLIENT_H
