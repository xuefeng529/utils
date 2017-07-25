#ifndef NET_TCPCLIENT_H
#define NET_TCPCLIENT_H

#include "base/Mutex.h"
#include "net/TcpConnection.h"
#include "net/SSLUtil.h"

#include <boost/noncopyable.hpp>

namespace net
{

class EventLoop;
class Connector;

typedef boost::function<void(const TcpConnectionPtr&)> HearbeatCallback;

class TcpClient : boost::noncopyable
{
public:
	TcpClient(EventLoop* loop,
			  const InetAddress& serverAddr,
			  const std::string& name,
			  time_t heartbeat = 0);

	~TcpClient(); 

    void enableSSL(const std::string& cacertFile,
                   const std::string& certFile,
                   const std::string& keyFile,
                   const std::string& passwd);

	void connect();
	void disconnect();

	/// 避免使用
	void send(const BufferPtr& message);
	void send(const char* message);
	void send(const std::string& message);
	void send(const void* message, size_t len);

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

	void setHearbeatCallback(const HearbeatCallback& cb)
	{ hearbeatCallback_ = cb; }
	
	/// 避免使用
	TcpConnectionPtr connection()
	{
		base::MutexLockGuard lock(mutex_);
		return connection_;
	}
	
	bool isConnected()
	{
		base::MutexLockGuard lock(mutex_);
		return static_cast<bool>(connection_);
	}

private:
	void newConnection(int sockfd);
	void connectingFailed();
	void removeConnection(const TcpConnectionPtr& conn);
	void connectInLoop();
	void disconnectInLoop();
	void handleHearbeat();
	void sendInLoop(const void* data, size_t len);
	void sendInLoop(const std::string& data);
	void sendBufferInLoop(const BufferPtr& data);

	static const time_t kMaxRetryDelayS = 30;
	static const time_t kInitRetryDelayS = 1;

	EventLoop* loop_;
	typedef boost::shared_ptr<Connector> ConnectorPtr;
	ConnectorPtr connector_;
	const std::string name_;
	const time_t heartbeat_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	HearbeatCallback hearbeatCallback_;
	bool connect_;
	int nextConnId_;
	base::MutexLock mutex_;
	TcpConnectionPtr connection_;
	time_t retryDelayS_;
	bool retry_;
    SSL_CTX* sslCtx_;
};

} // namespace net

#endif // NET_TCPCLIENT_H
