#include "net/TcpConnection.h"
#include "net/EventLoop.h"
#include "net/config.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <errno.h>

namespace net
{

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
	LOG_DEBUG << conn->localAddress().toIpPort() << " -> "
		<< conn->peerAddress().toIpPort() << " is "
		<< (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr&, net::Buffer* buffer)
{
	buffer->retrieveAll();
}

void TcpConnection::handleRead(struct bufferevent *bev, void *ctx)
{
	TcpConnection* conn = static_cast<TcpConnection*>(ctx);
	LOG_DEBUG << "TcpConnection::handleRead[" << conn->name_ << "]";
	assert(conn->inputBuffer_->length() != 0);
	conn->messageCallback_(conn->shared_from_this(), conn->inputBuffer_.get());
}

void TcpConnection::handleWrite(struct bufferevent *bev, void *ctx)
{
	TcpConnection* conn = static_cast<TcpConnection*>(ctx);
	LOG_DEBUG << "TcpConnection::handleWrite[" << conn->name_ << "]";
	assert(conn->outputBuffer_->length() == 0);
	conn->writing_ = false;
	if (conn->writeCompleteCallback_)
	{
		conn->writeCompleteCallback_(conn->shared_from_this());
	}

	if (conn->state_ == kDisconnecting)
	{
		conn->handleClose();
	}
}

void TcpConnection::handleEvent(struct bufferevent *bev, short events, void *ctx)
{
	TcpConnection* conn = static_cast<TcpConnection*>(ctx);
	if (events & BEV_EVENT_EOF)
	{
		LOG_DEBUG << "TcpConnection::handleEvent[" << conn->name_ << "]:" << " BEV_EVENT_EOF";
		conn->handleClose();
		return;
	}
	else if (events & BEV_EVENT_ERROR)
	{
		LOG_WARN << "TcpConnection::handleEvent[" << conn->name_ << "]:" << " BEV_EVENT_ERROR: "
			<< evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
		conn->handleError();
		return;
	}

	if (events & (BEV_EVENT_READING | BEV_EVENT_TIMEOUT))
	{
		LOG_DEBUG << "TcpConnection::handleEvent[" << conn->name_ << "]:" << " BEV_EVENT_READING|BEV_EVENT_TIMEOUT";
		conn->handleReadIdle();
	}
}

TcpConnection::TcpConnection(EventLoop* loop,
							 const std::string& name,
							 int sockfd,
							 const InetAddress& localAddr,
							 const InetAddress& peerAddr,
							 time_t readIdle)
	: loop_(loop),
	  name_(name),
	  sockfd_(sockfd),
	  localAddr_(localAddr),
	  peerAddr_(peerAddr),
	  readIdle_(readIdle),
	  state_(kConnecting),
	  writing_(false),
	  bev_(NULL)
{
	LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
		<< " fd=" << sockfd_;
}

TcpConnection::~TcpConnection()
{
	LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
		<< " fd=" << sockfd_ << " state=" << stateToString();
	assert(state_ == kDisconnected);
	if (bev_ != NULL)
	{
		bufferevent_free(bev_);
	}
}

void TcpConnection::enableReading()
{
	bufferevent_enable(bev_, EV_READ);
}

void TcpConnection::disableReading()
{
	bufferevent_disable(bev_, EV_READ);
}

void TcpConnection::enableWriting()
{
	bufferevent_enable(bev_, EV_WRITE);
}

void TcpConnection::disableWriting()
{
	bufferevent_disable(bev_, EV_WRITE);
}

void TcpConnection::disableAll()
{
	bufferevent_disable(bev_, EV_READ | EV_WRITE);
}

void TcpConnection::connectEstablished()
{
	loop_->assertInLoopThread();
	bev_ = bufferevent_socket_new(loop_->eventBase(), sockfd_, BEV_OPT_CLOSE_ON_FREE);
	if (bev_ == NULL)
	{
		LOG_ERROR << "bufferevent_socket_new of TcpConnection::connectEstablished[" << name_ << "]: "
			<< evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR());
		return;
	}
	
	setState(kConnected);
	bufferevent_setcb(bev_, handleRead, handleWrite, handleEvent, this);
	if (readIdle_ > 0)
	{
		struct timeval tv;
		bzero(&tv, sizeof(tv));
		tv.tv_sec = readIdle_;
		bufferevent_set_timeouts(bev_, &tv, NULL);
	}
	inputBuffer_.reset(new Buffer(bufferevent_get_input(bev_)));
	outputBuffer_.reset(new Buffer(bufferevent_get_output(bev_)));
	enableReading();
	connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
	loop_->assertInLoopThread();
	if (state_ == kConnected)
	{
		setState(kDisconnected);
		disableAll();
		connectionCallback_(shared_from_this());
	}
}

void TcpConnection::close()
{
	loop_->runInLoop(boost::bind(&TcpConnection::closeInLoop, shared_from_this()));
}

void TcpConnection::closeInLoop()
{
	loop_->assertInLoopThread();
	if (state_ == kConnected)
	{
		if (writing_)
		{
			LOG_DEBUG << "TcpConnection::closeInLoop[" << name_ << "]";
			state_ = kDisconnecting;
		}
		else
		{
			handleClose();
		}
	}
}

void TcpConnection::handleClose()
{
	loop_->assertInLoopThread();
	LOG_DEBUG << "TcpConnection::handleClose()[" << name_ << "]"  << " state = " << stateToString();
	if (state_ == kConnected || state_ == kDisconnecting)
	{
		setState(kDisconnected);
		disableAll();
		TcpConnectionPtr conn(shared_from_this());
		connectionCallback_(conn);
		closeCallback_(conn);
	}
}

void TcpConnection::handleReadIdle()
{
	loop_->assertInLoopThread();
	LOG_WARN << "TcpConnection::handleReadIdle[" << name_ << "]";
	handleClose();
}

void TcpConnection::send(const char* message)
{
	if (loop_->isInLoopThread())
	{
		sendInLoop(static_cast<const void*>(message), strlen(message));
	}
	else
	{
		loop_->queueInLoop(boost::bind(
			&TcpConnection::sendInLoop, shared_from_this(), std::string(message)));
	}
}

void TcpConnection::send(const std::string& message)
{
	if (loop_->isInLoopThread())
	{
		sendInLoop(message);
	}
	else
	{
		loop_->queueInLoop(boost::bind(
			&TcpConnection::sendInLoop, shared_from_this(), message));
	}
}

void TcpConnection::send(const void* message, size_t len)
{
	if (loop_->isInLoopThread())
	{
		sendInLoop(message, len);
	}
	else
	{
		loop_->queueInLoop(boost::bind(
			&TcpConnection::sendInLoop, shared_from_this(), std::string(static_cast<const char*>(message), len)));
	}
}

void TcpConnection::send(const BufferPtr& message)
{
	if (loop_->isInLoopThread())
	{
		sendBufferInLoop(message);
	}
	else
	{
		loop_->runInLoop(boost::bind(
			&TcpConnection::sendBufferInLoop, shared_from_this(), message));
	}
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
	loop_->assertInLoopThread();
	if (state_ == kConnected)
	{
		if (!writing_)
		{
			writing_ = true;
		}
		outputBuffer_->append(data, len);
	}
}

void TcpConnection::sendInLoop(const std::string& data)
{
	sendInLoop(static_cast<const void*>(data.data()), data.length());
}

void TcpConnection::sendBufferInLoop(const BufferPtr& data)
{
	loop_->assertInLoopThread();
	if (state_ == kConnected)
	{
		if (!writing_)
		{
			writing_ = true;
		}
		outputBuffer_->removeBuffer(data.get());
	}
}

void TcpConnection::handleError()
{
	handleClose();
}

const char* TcpConnection::stateToString() const
{
	switch (state_)
	{
	case kDisconnected:
		return "kDisconnected";
	case kConnecting:
		return "kConnecting";
	case kConnected:
		return "kConnected";
	case kDisconnecting:
		return "kDisconnecting";
	default:
		return "unknown state";
	}
}

} // namespace net
