#include "net/http/HttpClient.h"
#include "net/http/HttpContext.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

namespace net
{

HttpClient::HttpClient(EventLoop* loop,
		               const std::string& host,
		               uint16_t port,
					   bool retry,
		               const std::string& name)
{
	net::InetAddress serverAddr(port);
	if (!net::InetAddress::resolve(host.c_str(), &serverAddr))
	{
		LOG_FATAL << host;
	}

	client_.reset(new TcpClient(loop, serverAddr, name));
	client_->setConnectionCallback(
		boost::bind(&HttpClient::handleConnection, this, _1));
	client_->setMessageCallback(
		boost::bind(&HttpClient::handleMessage, this, _1, _2));
	if (retry)
	{
		client_->enableRetry();
	}
}

void HttpClient::handleConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected() && sendRequestCallback_)
	{
		HttpContext context(conn, HttpContext::kResponse);
		context.setResponseCallback(boost::bind(&HttpClient::handleResponse, this, _1, _2));
		conn->setContext(context);
		HttpRequest request;
		sendRequestCallback_(&request);
		BufferPtr buffer(new Buffer());
		request.appendToBuffer(buffer.get());
		conn->send(buffer);
	}
}

void HttpClient::handleMessage(const TcpConnectionPtr& conn, Buffer* buffer)
{
	HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());
	if (!context->parse(buffer))
	{
		conn->close();
	}
}

void HttpClient::handleResponse(const TcpConnectionPtr& conn, const HttpResponse& response)
{
	if (responseCallback_)
	{
		responseCallback_(response);
		if (!response.closeConnection() && sendRequestCallback_)
		{
			HttpRequest request;
			sendRequestCallback_(&request);
			BufferPtr buffer(new Buffer());
			request.appendToBuffer(buffer.get());
			conn->send(buffer);
		}
	}
}

} // namespace net
