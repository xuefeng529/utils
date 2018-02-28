#ifndef NET_HTTP_HTTPCLIENT_H
#define NET_HTTP_HTTPCLIENT_H

#include "net/TcpClient.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"

namespace base
{
class CountDownLatch;
}

namespace net
{

class EventLoop;
class EventLoopThread;

class HttpClient : boost::noncopyable
{
public:
    HttpClient(bool keepalive = true, SslContext* sslCtx = NULL);
    ~HttpClient();

    HttpResponse request(const std::string& url, HttpRequest::Method method, const std::string& body = std::string());

private:
    void handleConnection(const TcpConnectionPtr& conn, const net::BufferPtr& buffer);
	void handleMessage(const TcpConnectionPtr& conn, Buffer* buffer);
	void handleResponse(const TcpConnectionPtr& conn, const HttpResponse& response);
   
    boost::scoped_ptr<EventLoopThread> loopThread_;
    EventLoop* loop_;
	boost::scoped_ptr<TcpClient> client_;
    boost::scoped_ptr<base::CountDownLatch> requestLatch_;
    boost::scoped_ptr<HttpResponse> response_;
    std::string lastHost_;
    bool keepalive_;
    SslContext* sslCtx_;
};

} // namespace net

#endif // NET_HTTP_HTTPCLIENT_H
