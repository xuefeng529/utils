#ifndef NET_TCPSERVER_H
#define NET_TCPSERVER_H

#include "base/Atomic.h"
#include "base/Mutex.h"
#include "net/TcpConnection.h"

#include <boost/scoped_ptr.hpp>
#include <boost/unordered_set.hpp>
#include <boost/circular_buffer.hpp>
#include <map>

namespace net
{

class Buffer;
class EventLoop;
class Acceptor;
class EventLoopThreadPool;
class SslContext;

class TcpServer : boost::noncopyable
{
public:
	TcpServer(EventLoop* loop,
			  const InetAddress& listenAddr,
			  const std::string& name,
			  time_t readIdle = 0,
              SslContext* sslCtx = NULL);

	~TcpServer();

	typedef boost::function<void(EventLoop*)> ThreadInitCallback;
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{ threadInitCallback_ = cb; }

	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }

	const std::string& ipPort() const { return ipPort_; }
	const std::string& name() const { return name_; }
	void setThreadNum(int numThreads);
	void start();
    EventLoop* getLoop() const { return loop_; }
    boost::shared_ptr<EventLoopThreadPool> threadPool() const
    { return threadPool_; }

private:
	void newConnection(int sockfd, const InetAddress& peerAddr);
	void removeConnection(const TcpConnectionPtr& conn);
	void removeConnectionInLoop(const TcpConnectionPtr& conn);

	void handleConnection(const net::TcpConnectionPtr& conn);
	void handleMessage(const net::TcpConnectionPtr& conn, Buffer* buffer);
	void handleWriteComplete(const net::TcpConnectionPtr& conn);
	//void handleIdle();
	void dumpConnectionBuckets() const;

	EventLoop* loop_;
	const std::string ipPort_;
	const std::string name_;
	const time_t readIdle_;
	boost::scoped_ptr<Acceptor> acceptor_;
	boost::shared_ptr<EventLoopThreadPool> threadPool_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	int nextConnId_;
	typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;
	ConnectionMap connections_;
	base::AtomicInt32 started_;
	ThreadInitCallback threadInitCallback_;
    SslContext* sslCtx_;
	/*typedef boost::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
	struct Entry
	{
	explicit Entry(const WeakTcpConnectionPtr& weakConn)
	: weakConn_(weakConn)
	{ }

	~Entry()
	{
	TcpConnectionPtr conn = weakConn_.lock();
	if (conn)
	{
	conn->close();
	}
	}

	WeakTcpConnectionPtr weakConn_;
	};

	typedef boost::shared_ptr<Entry> EntryPtr;
	typedef boost::weak_ptr<Entry> WeakEntryPtr;
	typedef boost::unordered_set<EntryPtr> Bucket;
	typedef boost::circular_buffer<Bucket> WeakConnectionList;

	mutable base::MutexLock mutex_;
	WeakConnectionList connectionBuckets_;*/
};

} // namespace net

#endif // NET_TCPSERVER_H
