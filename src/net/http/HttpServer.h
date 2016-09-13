#ifndef NET_HTTP_HTTPSERVER_H
#define NET_HTTP_HTTPSERVER_H

#include "net/TcpServer.h"

namespace net
{

class HttpRequest;
class HttpResponse;

class HttpServer : boost::noncopyable
{
public:
	typedef boost::function<void(const TcpConnectionPtr&, const HttpRequest&)> HttpCallback;

	HttpServer(EventLoop* loop,
			   const InetAddress& listenAddr,
			   const std::string& name);

	typedef boost::function<void(EventLoop*)> ThreadInitCallback;
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{ threadInitCallback_ = cb; }

	void setHttpCallback(const HttpCallback& cb)
	{ httpCallback_ = cb; }

	void setThreadNum(int numThreads)
	{
		server_.setThreadNum(numThreads);
	}

	void start();

private:
	void onConnection(const TcpConnectionPtr& conn);
	void onMessage(const TcpConnectionPtr& conn, Buffer* buf);

	TcpServer server_;
	HttpCallback httpCallback_;
	ThreadInitCallback threadInitCallback_;
};

} // namespace net

#endif // NET_HTTP_HTTPSERVER_H
