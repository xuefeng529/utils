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

TcpClient::TcpClient(EventLoop* loop,
					 const InetAddress& serverAddr,
					 const std::string& name,
					 time_t connectingExpire,
					 time_t heartbeat)
	: loop_(loop),
	  connector_(new Connector(loop, serverAddr)),
	  name_(name),
	  connectingExpire_(connectingExpire < kMaxRetryDelayS ? kMaxRetryDelayS : connectingExpire),
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
	connector_->start();
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
	if (connection_)
	{
		connection_->send(message);
	}
}

void TcpClient::send(const std::string& message)
{
	if (connection_)
	{
		connection_->send(message);
	}
}

void TcpClient::send(const void* message, size_t len)
{
	if (connection_)
	{
		connection_->send(message, len);
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
	connection_ = conn;
	conn->connectEstablished();
}

void TcpClient::connectingFailed()
{
	loop_->assertInLoopThread();
	if (retryDelayS_ < connectingExpire_)
	{
		LOG_INFO << "TcpClient::connectingFailed[" << name_ << "]: Reconnecting to "
			<< connector_->serverAddress().toIpPort();
		loop_->runAfter(retryDelayS_,
			boost::bind(&Connector::restart, connector_));
		retryDelayS_ *= 2;
	}
	else
	{
		if (connectingExpireCallback_)
		{
			connectingExpireCallback_();
		}
	}
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	LOG_DEBUG << "TcpClient::removeConnection[" << name_ << "]";
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());
	retryDelayS_ = kInitRetryDelayS;
	connection_.reset();
	loop_->runInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
	if (connect_ && retry_)
	{
		LOG_DEBUG << "TcpClient::removeConnection[" << name_ << "]: Reconnecting to "
			<< connector_->serverAddress().toIpPort();
		if (retryDelayS_ < connectingExpire_)
		{
			loop_->runAfter(retryDelayS_,
				boost::bind(&Connector::restart, connector_));
			retryDelayS_ *= 2;
		}
		else
		{
			if (connectingExpireCallback_)
			{
				connectingExpireCallback_();
			}
		}
	}
}

} // namespace net
