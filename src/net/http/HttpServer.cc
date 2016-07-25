#include "net/http/HttpServer.h"
#include "net/http/HttpContext.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"
#include "net/Buffer.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

namespace net
{
namespace detail
{

void defaultHttpCallback(const net::TcpConnectionPtr& conn, const net::HttpRequest& request)
{
	const std::string& connection = request.getHeader("Connection");
	bool close = connection == "close" ||
		(request.getVersion() == net::HttpRequest::kHttp10 && connection != "Keep-Alive");
	net::HttpResponse response(close);
	response.setStatusCode(HttpResponse::k404NotFound);
	response.setStatusMessage("Not Found");
	net::BufferPtr buf(new net::Buffer());
	response.appendToBuffer(buf.get());
	conn->send(buf);
	if (response.closeConnection())
	{
		conn->close();
	}
} 

}// namespace detail

HttpServer::HttpServer(EventLoop* loop,
					   const InetAddress& listenAddr,
					   const std::string& name)
	: server_(loop, listenAddr, name),
	  httpCallback_(detail::defaultHttpCallback)
{
	server_.setConnectionCallback(
		boost::bind(&HttpServer::onConnection, this, _1));
	server_.setMessageCallback(
		boost::bind(&HttpServer::onMessage, this, _1, _2));
}

void HttpServer::start()
{
	LOG_WARN << "HttpServer[" << server_.name()
		<< "] starts listenning on " << server_.ipPort();
	server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		conn->setContext(HttpContext());
	}
}

void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf)
{
	HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());

	if (!context->parseRequest(buf))
	{
		conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
		conn->close();
	}

	if (context->gotAll())
	{
		httpCallback_(conn, context->request());
		context->reset();
	}
}

} // namespace net
