#include "net/http2/HttpServer.h"
#include "net/http2/HttpContext.h"
#include "net/http2/HttpRequest.h"
#include "net/http2/HttpResponse.h"
#include "net/Buffer.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

namespace net
{
namespace detail
{

void defaultHttpCallback(const HttpRequest& request, HttpResponse* response)
{
	const std::string& connection = request.getHeader("Connection");
	bool close = connection == "close" ||
		(request.version() == net::HttpRequest::kHttp10 && connection != "Keep-Alive");
	response->setStatusCode(HttpResponse::k404NotFound);
	response->setStatusMessage("Not Found");
	response->setCloseConnection(close);
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

void HttpServer::start()
{
	LOG_WARN << "HttpServer[" << server_.name()
		<< "] starts listenning on " << server_.ipPort();
	server_.setThreadInitCallback(threadInitCallback_);
	server_.start();
}

void HttpServer::handleConnection(const TcpConnectionPtr& conn)
{
	HttpContextPtr context(new HttpContext(HttpContext::kRequest));
	context->setRequestCallback(boost::bind(&HttpServer::handleRequest, this, conn, _1));
	conn->setContext(context);
}

void HttpServer::handleMessage(const TcpConnectionPtr& conn, Buffer* buffer)
{
	HttpContextPtr context = boost::any_cast<HttpContextPtr>(conn->getContext());
	if (!context->parse(buffer))
	{
		conn->close();
	}
}

void HttpServer::handleRequest(const TcpConnectionPtr& conn, const HttpRequest& request)
{
	if (requestCallback_)
	{
		const std::string& connection = request.getHeader("Connection");
		bool close = connection == "close" ||
			(request.version() == HttpRequest::kHttp10 && connection != "Keep-Alive");
		HttpResponse response;
		response.setCloseConnection(close);
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
