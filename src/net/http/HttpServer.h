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
	typedef boost::function<void(EventLoop*)> ThreadInitCallback;
	typedef boost::function<void(const HttpRequest&, HttpResponse*)> RequestCallback;

	HttpServer(EventLoop* loop,
			   const InetAddress& listenAddr,
			   const std::string& name,
               SslContext* sslCtx = NULL);   
	
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{ threadInitCallback_ = cb; }

	void setRequestCallback(const RequestCallback& cb)
	{ requestCallback_ = cb; }

	void setThreadNum(int numThreads)
	{ server_.setThreadNum(numThreads); }

	void start();

    EventLoop* getLoop() const { return server_.getLoop(); }
    boost::shared_ptr<EventLoopThreadPool> threadPool() const
    { return server_.threadPool(); }

private:
	void handleConnection(const TcpConnectionPtr& conn);
	void handleMessage(const TcpConnectionPtr& conn, Buffer* buffer);
	void handleRequest(const TcpConnectionPtr& conn, const HttpRequest& request);

    TcpServer server_;
	RequestCallback requestCallback_;
	ThreadInitCallback threadInitCallback_;
};

} // namespace net

#endif // NET_HTTP_HTTPSERVER_H
