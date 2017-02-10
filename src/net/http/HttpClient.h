#ifndef NET_HTTP_HTTPCLIENT_H
#define NET_HTTP_HTTPCLIENT_H

#include "net/TcpClient.h"

namespace net
{

class HttpRequest;
class HttpResponse;

class HttpClient : boost::noncopyable
{
public:
	typedef boost::function<void(HttpRequest*)> SendRequestCallback;
	typedef boost::function<void(const HttpResponse&)> ResponseCallback;

	HttpClient(EventLoop* loop,
		       const std::string& host,
			   uint16_t port = 80,
			   bool retry = false,
		       const std::string& name = std::string());

	void setSendRequestCallback(const SendRequestCallback& cb)
	{ sendRequestCallback_ = cb; }

	void setResponseCallback(const ResponseCallback& cb)
	{ responseCallback_ = cb; }

	void connect()
	{ client_->connect(); }

private:
	void handleConnection(const TcpConnectionPtr& conn);
	void handleMessage(const TcpConnectionPtr& conn, Buffer* buffer);
	void handleResponse(const TcpConnectionPtr& conn, const HttpResponse& response);

	boost::scoped_ptr<TcpClient> client_;
	std::string host_;
	SendRequestCallback sendRequestCallback_;
	ResponseCallback responseCallback_;
};

} // namespace net

#endif // NET_HTTP_HTTPCLIENT_H
