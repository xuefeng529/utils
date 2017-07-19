#ifndef NET_TCPCONNECTION_H
#define NET_TCPCONNECTION_H

#include "net/InetAddress.h"
#include "net/Buffer.h"
#include "net/SSLUtil.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/any.hpp>

struct evbuffer;
struct bufferevent;

namespace net
{
	
class EventLoop;
class InetAddress;
class Buffer;
class TcpConnection;

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef boost::function<void(const TcpConnectionPtr&)> ConnectionCallback;
typedef boost::function <void(const TcpConnectionPtr&, Buffer* buffer)> MessageCallback;
typedef boost::function<void(const TcpConnectionPtr&)> WriteCompleteCallback;
typedef boost::function<void(const TcpConnectionPtr&)> CloseCallback;

void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn, net::Buffer* buffer);

class TcpConnection : boost::noncopyable,
					  public boost::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* loop,
				  const std::string& name,
				  int sockfd,
				  const InetAddress& localAddr,
				  const InetAddress& peerAddr,
				  time_t readIdle = 0);

	~TcpConnection();

	bool connected() const
	{ return state_ == kConnected; }

	bool disconnected() const
	{ return state_ == kDisconnected; }

	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }

	void setCloseCallback(const CloseCallback& cb)
	{ closeCallback_ = cb; }

	void setContext(const boost::any& context)
	{ context_ = context; }

	const boost::any& getContext() const
	{ return context_; }

	boost::any* getMutableContext()
	{ return &context_; }

	const InetAddress& localAddress() const { return localAddr_; }
	const InetAddress& peerAddress() const { return peerAddr_; }

	void send(const char* message);
	void send(const std::string& message);
	void send(const void* message, size_t len);
	void send(const BufferPtr& message);

	void shutdown();
	void close();
	void forceClose();
	
	void connectEstablished();
    enum SSLState { kSSLAccepting, kSSLConnecting };
    void connectEstablished(SSL* ssl, SSLState state);
	void connectDestroyed();
	EventLoop* getLoop() const { return loop_; }
	const std::string& name() const { return name_; }

private:
	static void handleRead(struct bufferevent *bev, void *ctx);
	static void handleWrite(struct bufferevent *bev, void *ctx);
	static void handleEvent(struct bufferevent *bev, short events, void *ctx);

	enum State { kDisconnected, kConnecting, kConnected, kDisconnecting };
	enum CloseType { kUnknow, kShutdown, kClose };

	void sendInLoop(const void* data, size_t len);
	void sendInLoop(const std::string& data);
	void sendBufferInLoop(const BufferPtr& data);
	void setState(State s) { state_ = s; }
	void handleError();
	void handleClose();
	void handleReadIdle();
	void enableReading();
	void disableReading();
	void enableWriting();
	void disableWriting();
	void disableAll();
	void shutdownInLoop();
	void closeInLoop();
	void forceCloseInLoop();
	const char* stateToString() const;

	EventLoop* loop_;
	const std::string name_;
	const int sockfd_;
	const InetAddress localAddr_;
	const InetAddress peerAddr_;
	const time_t readIdle_;
	boost::scoped_ptr<Buffer> inputBuffer_;
	boost::scoped_ptr<Buffer> outputBuffer_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	CloseCallback closeCallback_;
	State state_;
	CloseType closeType_;
	boost::any context_;
	struct bufferevent* bev_;
    SSL* ssl_;
};

} // namespace net

#endif // NET_TCPCONNECTION_H
