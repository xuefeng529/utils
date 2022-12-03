#include "net/http/HttpServer.h"
#include "net/http/HttpContext.h"
#include "net/SslContext.h"
#include "net/Buffer.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

namespace net
{
namespace detail
{

void defaultHttpCallback(const HttpRequest& request, HttpResponse* response)
{
	response->setStatusCode(HttpResponse::k404NotFound);
	response->setStatusMessage("Not Found");
	response->setCloseConnection(true);
} 

}// namespace detail

HttpServer::HttpServer(EventLoop* loop,
					   const InetAddress& listenAddr,
					   const std::string& name,
                       SslContext* sslCtx)
	: server_(loop, listenAddr, name, 0, sslCtx),
	  requestCallback_(detail::defaultHttpCallback)
{
	server_.setConnectionCallback(
		boost::bind(&HttpServer::handleConnection, this, _1));
	server_.setMessageCallback(
		boost::bind(&HttpServer::handleMessage, this, _1, _2));
}

void HttpServer::start()
{
	LOG_INFO << "HttpServer[" << server_.name()
		<< "] starts listening on " << server_.ipPort();
	server_.setThreadInitCallback(threadInitCallback_);
	server_.start();
}

void HttpServer::handleConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		HttpContext context(conn, HttpContext::kRequest);
		context.setRequestCallback(boost::bind(&HttpServer::handleRequest, this, _1, _2));
		conn->setContext(context);
	}
}

void HttpServer::handleMessage(const TcpConnectionPtr& conn, Buffer* buffer)
{
	HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());
	if (!context->parse(buffer))
	{
		conn->close();
	}
}

void HttpServer::handleRequest(const TcpConnectionPtr& conn, const HttpRequest& request)
{
	if (requestCallback_)
	{
		HttpResponse response;
		response.setCloseConnection(request.closeConnection());
		requestCallback_(request, &response);
		BufferPtr buffer(new Buffer());
		response.appendToBuffer(buffer.get());
		HttpResponse::ChunkedCallback chunkedCb = response.getChunkedCallback();
		if (chunkedCb)
		{
			conn->setWriteCompleteCallback(boost::bind(&HttpServer::handleWriteComplete,
				this, response.closeConnection(), chunkedCb, _1));
			conn->send(buffer);
		}
		else
		{
			conn->send(buffer);
			if (response.closeConnection())
			{
				conn->close();
			}
		}
	}
}

void HttpServer::handleWriteComplete(bool close,
									 const HttpResponse::ChunkedCallback& chunkedCb,									 
									 const TcpConnectionPtr& conn)
{
	assert(chunkedCb);
	bool endChunked = false;
	BufferPtr buffer(new Buffer());
	if (!chunkedCb(buffer.get(), &endChunked))
	{
		conn->close();
		return;
	}
	
	if (buffer->length() > 0)
	{
		conn->send(buffer);
	}

	if (endChunked)
	{
		conn->setWriteCompleteCallback(NULL);
		if (close)
		{
			conn->close();
		}
	}
}

} // namespace net
