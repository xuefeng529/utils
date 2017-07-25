#include "net/TcpClient.h"
#include "net/config.h"
#include "net/EventLoop.h"
#include "net/Connector.h"
#include "base/Logging.h"

#include <boost/bind.hpp>
#include <algorithm>

#include <errno.h>

namespace net
{

const time_t TcpClient::kMaxRetryDelayS;
const time_t TcpClient::kInitRetryDelayS;

TcpClient::TcpClient(EventLoop* loop,
					 const InetAddress& serverAddr,
					 const std::string& name,
					 time_t heartbeat)
	: loop_(loop),
	  connector_(new Connector(loop, serverAddr)),
	  name_(name),
	  heartbeat_(heartbeat),
	  connectionCallback_(defaultConnectionCallback),
	  messageCallback_(defaultMessageCallback),
	  connect_(false),
	  nextConnId_(1),
	  retryDelayS_(kInitRetryDelayS),
	  retry_(false)
{
	LOG_DEBUG << "TcpClient::ctor[" << name_
		<< "]: connector " << get_pointer(connector_);
	connector_->setNewConnectionCallback(
		boost::bind(&TcpClient::newConnection, this, _1));

	connector_->setConnectingFailedCallback(
		boost::bind(&TcpClient::connectingFailed, this));

	if (heartbeat_ > 0)
	{
		loop_->runEvery(heartbeat_, boost::bind(&TcpClient::handleHearbeat, this));
	}
}

TcpClient::~TcpClient()
{
	LOG_DEBUG << "TcpClient::dtor[" << name_
		<< "] - connector " << get_pointer(connector_);
    if (sslCtx_ != NULL)
    {
        ssl::release(sslCtx_);
    }
}

void TcpClient::enableSSL(const std::string& cacertFile,
                          const std::string& certFile,
                          const std::string& keyFile,
                          const std::string& passwd)
{
    sslCtx_ = ssl::init(cacertFile, certFile, keyFile, passwd);
    if (sslCtx_ == NULL)
    {
        LOG_FATAL << "ssl::init: " << ssl::error();
    }
}

void TcpClient::connect()
{
	loop_->runInLoop(boost::bind(&TcpClient::connectInLoop, this));
}

void TcpClient::connectInLoop()
{
	loop_->assertInLoopThread();
	connect_ = true;
	retryDelayS_ = kInitRetryDelayS;
	connector_->restart();
}

void TcpClient::disconnect()
{
	loop_->runInLoop(boost::bind(&TcpClient::disconnectInLoop, this));
}

void TcpClient::disconnectInLoop()
{
	loop_->assertInLoopThread();
	if (connect_)
	{
		connect_ = false;
		if (connection_)
		{
			connection_->shutdown();
		}
	}
}

void TcpClient::send(const char* message)
{
	if (loop_->isInLoopThread())
	{
		sendInLoop(static_cast<const void*>(message), strlen(message));
	}
	else
	{
		loop_->queueInLoop(boost::bind(
			&TcpClient::sendInLoop, this, std::string(message)));
	}
}

void TcpClient::send(const std::string& message)
{
	if (loop_->isInLoopThread())
	{
		sendInLoop(message);
	}
	else
	{
		loop_->queueInLoop(boost::bind(
			&TcpClient::sendInLoop, this, message));
	}
}

void TcpClient::send(const void* message, size_t len)
{
	if (loop_->isInLoopThread())
	{
		sendInLoop(message, len);
	}
	else
	{
		loop_->queueInLoop(boost::bind(
			&TcpClient::sendInLoop, this, std::string(static_cast<const char*>(message), len)));
	}
}

void TcpClient::send(const BufferPtr& message)
{
	if (loop_->isInLoopThread())
	{
		sendBufferInLoop(message);
	}
	else
	{
		loop_->runInLoop(boost::bind(
			&TcpClient::sendBufferInLoop, this, message));
	}
}

void TcpClient::sendInLoop(const void* data, size_t len)
{
	loop_->assertInLoopThread();
	if (connection_)
	{
		connection_->send(data, len);
	}
}

void TcpClient::sendInLoop(const std::string& data)
{
	loop_->assertInLoopThread();
	if (connection_)
	{
		connection_->send(data);
	}
}

void TcpClient::sendBufferInLoop(const BufferPtr& data)
{
	loop_->assertInLoopThread();
	if (connection_)
	{
		connection_->send(data);
	}
}

void TcpClient::handleHearbeat()
{
	if (hearbeatCallback_ && connection_)
	{
		hearbeatCallback_(connection_);
	}
}

void TcpClient::newConnection(int sockfd)
{
	loop_->assertInLoopThread();
	retryDelayS_ = kInitRetryDelayS;
	struct sockaddr_in peeraddr;
	bzero(&peeraddr, sizeof(peeraddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
	if (getpeername(sockfd, reinterpret_cast<struct sockaddr*>(&peeraddr), &addrlen) < 0)
	{
		LOG_SYSERR << "getpeername of TcpClient::newConnection[" << name_ << "]: "
			<< base::strerror_tl(errno);
		return;
	}

	InetAddress peerAddr(peeraddr);
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;
	struct sockaddr_in localaddr;
	bzero(&localaddr, sizeof(localaddr));
	addrlen = static_cast<socklen_t>(sizeof(localaddr));
	if (getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&localaddr), &addrlen) < 0)
	{
		LOG_SYSERR << "getsockname of TcpClient::newConnection[" << connName << "]: "
			<< base::strerror_tl(errno);
		return;
	}

	InetAddress localAddr(localaddr);
	TcpConnectionPtr conn(new TcpConnection(
		loop_, connName, sockfd, localAddr, peerAddr, 0));
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(boost::bind(&TcpClient::removeConnection, this, _1));
    
	{
		base::MutexLockGuard lock(mutex_);
		connection_ = conn;
	}
	
    SSL* ssl = NULL;
    if (sslCtx_ != NULL)
    {
        ssl = ssl::open(sslCtx_);
        conn->connectEstablished(ssl, TcpConnection::kSSLConnecting);
    }
    else
    {
        conn->connectEstablished();
    }
}

void TcpClient::connectingFailed()
{
	loop_->assertInLoopThread();
	LOG_ERROR << "connect failed[" << name_ << "]";
	if (connect_ && retry_)
	{
		LOG_INFO << "Reconnecting to " << connector_->serverAddress().toIpPort() << "[" << name_ << "]";
		loop_->runAfter(retryDelayS_, boost::bind(&Connector::restart, connector_));
		retryDelayS_ = std::min(retryDelayS_ * 2, kMaxRetryDelayS);
	}
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	LOG_DEBUG << "TcpClient::removeConnection[" << name_ << "]";
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());
	retryDelayS_ = kInitRetryDelayS;

	{
		base::MutexLockGuard lock(mutex_);
		connection_.reset();
	}
	
	if (connect_ && retry_)
	{
		connectingFailed();
	}
}

} // namespace net
