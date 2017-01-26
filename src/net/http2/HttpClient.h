#ifndef NET_HTTP_HTTPCLIENT_H
#define NET_HTTP_HTTPCLIENT_H

#include "net/TcpClient.h"
#include "net/http2/HttpRequest.h"

namespace net
{

class HttpResponse;

class HttpClient : boost::noncopyable
{
public:
	typedef boost::function<void(HttpRequest*)> SendRequestCallback;
	typedef boost::function<void(const HttpResponse&)> ResponseCallback;

	HttpClient(EventLoop* loop, 
		       const InetAddress& serverAddr, 
		       const std::string& name, 
		       uint64_t connectingExpire);

	void setSendRequestCallback(const SendRequestCallback& cb)
	{ sendRequestCallback_ = cb; }

	void setResponseCallback(const ResponseCallback& cb)
	{ responseCallback_ = cb; }

	void connect()
	{ client_.connect(); }

private:
	void handleConnection(const TcpConnectionPtr& conn);
	void handleMessage(const TcpConnectionPtr& conn, Buffer* buffer);
	void handleResponse(const TcpConnectionPtr& conn, const HttpResponse& response);

	TcpClient client_;
	SendRequestCallback sendRequestCallback_;
	ResponseCallback responseCallback_;
};

} // namespace net

#endif // NET_HTTP_HTTPCLIENT_H
