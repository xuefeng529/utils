#include "net/http/HttpServer.h"
#include "net/http/HttpContext.h"
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
					   const std::string& name)
	: server_(loop, listenAddr, name),
	  requestCallback_(detail::defaultHttpCallback)
{
	server_.setConnectionCallback(
		boost::bind(&HttpServer::handleConnection, this, _1));
	server_.setMessageCallback(
		boost::bind(&HttpServer::handleMessage, this, _1, _2));
}

void HttpServer::enableSSL(const std::string& cacertFile, const std::string& certFile, const std::string& keyFile)
{
    server_.enableSSL(cacertFile, certFile, keyFile);
}

void HttpServer::start()
{
	LOG_WARN << "HttpServer[" << server_.name()
		<< "] starts listenning on " << server_.ipPort();
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
		conn->send(buffer);
		if (response.closeConnection())
		{
			conn->close();
		}
	}
}

} // namespace net
