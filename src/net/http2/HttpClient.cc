#include "net/http2/HttpClient.h"
#include "net/http2/HttpContext.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

namespace net
{

HttpClient::HttpClient(EventLoop* loop, 
		               const InetAddress& serverAddr, 
		               const std::string& name, 
		               uint64_t connectingExpire)
	: client_(loop, serverAddr, name, connectingExpire, 0)
{
	client_.setConnectionCallback(
		boost::bind(&HttpClient::handleConnection, this, _1));
	client_.setMessageCallback(
		boost::bind(&HttpClient::handleMessage, this, _1, _2));
}

void HttpClient::handleConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		HttpContextPtr context(new HttpContext(HttpContext::kResponse));
		context->setResponseCallback(boost::bind(&HttpClient::handleResponse, this, conn, _1));
		conn->setContext(context);
		HttpRequest request;
		if (sendRequestCallback_)
		{
			sendRequestCallback_(&request);
			BufferPtr buffer(new Buffer());
			request.appendToBuffer(buffer.get());
			conn->send(buffer);
		}
	}
}

void HttpClient::handleMessage(const TcpConnectionPtr& conn, Buffer* buffer)
{
	HttpContextPtr context = boost::any_cast<HttpContextPtr>(conn->getContext());
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
	}
}

} // namespace net
