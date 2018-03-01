#include "net/TcpServer.h"
#include "net/config.h"
#include "net/SslContext.h"
#include "net/Ssl.h"
#include "net/Acceptor.h"
#include "net/EventLoop.h"
#include "net/EventLoopThreadPool.h"
#include "net/Buffer.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <errno.h>

namespace net
{

TcpServer::TcpServer(EventLoop* loop,
					 const InetAddress& listenAddr,
					 const std::string& name,
					 time_t readIdle,
                     SslContext* sslCtx)
	: loop_(loop),
	  ipPort_(listenAddr.toIp()),
	  name_(name),
	  readIdle_(readIdle),
	  acceptor_(new Acceptor(loop, listenAddr)),
	  threadPool_(new EventLoopThreadPool(loop, name)),
	  connectionCallback_(net::defaultConnectionCallback),
	  messageCallback_(net::defaultMessageCallback),
	  nextConnId_(1),
      sslCtx_(sslCtx)
	  //connectionBuckets_(readTimeout)
{
	LOG_DEBUG << "TcpServer::ctor[" << name_ << "]";
	acceptor_->setNewConnectionCallback(
		boost::bind(&TcpServer::newConnection, this, _1, _2));
	/*if (readTimeout_ > 0)
	{
	loop_->runEvery(1, boost::bind(&TcpServer::handleIdle, this));
	connectionBuckets_.resize(idleSeconds);
	}*/
}

TcpServer::~TcpServer()
{
	loop_->assertInLoopThread();
	LOG_DEBUG << "TcpServer::dtor[" << name_ << "]";
	for (ConnectionMap::iterator it(connections_.begin());
		it != connections_.end(); ++it)
	{
		TcpConnectionPtr conn = it->second;
		it->second.reset();
		conn->getLoop()->runInLoop(
			boost::bind(&TcpConnection::connectDestroyed, conn));
	}   
}

void TcpServer::setThreadNum(int numThreads)
{
	assert(0 <= numThreads);
	threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
	if (started_.getAndSet(1) == 0)
	{
		threadPool_->start(threadInitCallback_);
		loop_->runInLoop(boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
	}
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	loop_->assertInLoopThread();
	EventLoop* ioLoop = threadPool_->getNextLoop();
	char buf[64];
	snprintf(buf, sizeof(buf), "-%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;
	LOG_DEBUG << "TcpServer::newConnection[" << name_
		<< "] - new connection [" << connName
		<< "] from " << peerAddr.toIpPort();
	struct sockaddr_in sin;
	bzero(&sin, sizeof(sin));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(sin));
	if (getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&sin), &addrlen) < 0)
	{
		LOG_ERROR << "getsockname of TcpServer::newConnection[" << connName << "]: "
			<< base::strerror_tl(errno);		
	}
	InetAddress localAddr(sin);
	TcpConnectionPtr conn(new TcpConnection(
		ioLoop, connName, sockfd, localAddr, peerAddr, readIdle_));
	connections_[connName] = conn;
	conn->setConnectionCallback(boost::bind(&TcpServer::handleConnection, this, _1));
	conn->setMessageCallback(boost::bind(&TcpServer::handleMessage, this, _1, _2));
	conn->setWriteCompleteCallback(boost::bind(&TcpServer::handleWriteComplete, this, _1));
	conn->setCloseCallback(boost::bind(&TcpServer::removeConnection, this, _1));    
    if (sslCtx_ != NULL)
    {        
        ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, 
            conn, new Ssl(*sslCtx_), TcpConnection::kSslAccepting));
    }
    else
    {
        ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
    }
}

void TcpServer::handleConnection(const net::TcpConnectionPtr& conn)
{
	/*if (readTimeout_ > 0)
	{
	if (conn->connected())
	{
	EntryPtr entry(new Entry(conn));

	{
	base::MutexLockGuard lock(mutex_);
	connectionBuckets_.back().insert(entry);
	}

	WeakEntryPtr weakEntry(entry);
	conn->setOtherContext(weakEntry);
	}
	}*/
	
	connectionCallback_(conn);
}

void TcpServer::handleMessage(const net::TcpConnectionPtr& conn, Buffer* buffer)
{
	/*if (readTimeout_ > 0)
	{
	assert(!conn->getOtherContext().empty());
	WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getOtherContext()));
	EntryPtr entry(weakEntry.lock());
	if (entry)
	{
	base::MutexLockGuard lock(mutex_);
	connectionBuckets_.back().insert(entry);
	}
	}*/
	
	messageCallback_(conn, buffer);
}

//void TcpServer::handleIdle()
//{
//	base::MutexLockGuard lock(mutex_);
//	connectionBuckets_.push_back(Bucket());
//}

void TcpServer::handleWriteComplete(const net::TcpConnectionPtr& conn)
{
	LOG_DEBUG << "TcpServer::handleWriteComplete[" << name_
		<< "] - connection " << conn->name();
	if (writeCompleteCallback_)
	{
		writeCompleteCallback_(conn);
	}
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	LOG_DEBUG << "TcpServer::removeConnectionInLoop[" << name_
		<< "]: connection " << conn->name();
	size_t n = connections_.erase(conn->name());
	(void)n;
	assert(n == 1);
}

} // namespace net
