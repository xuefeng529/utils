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
	HttpClient();
    ~HttpClient();
    void enableSSL(const std::string& cacertFile, const std::string& certFile, const std::string& keyFile);
    HttpResponse request(const std::string& url, HttpRequest::Method method = HttpRequest::kGet, bool keepalive = true);

private:
    void handleConnection(const TcpConnectionPtr& conn, const HttpRequest& request);
	void handleMessage(const TcpConnectionPtr& conn, Buffer* buffer);
	void handleResponse(const TcpConnectionPtr& conn, const HttpResponse& response);
   
    boost::scoped_ptr<EventLoopThread> loopThread_;
    EventLoop* loop_;
	boost::scoped_ptr<TcpClient> client_;
    boost::scoped_ptr<base::CountDownLatch> requestLatch_;
    boost::scoped_ptr<HttpResponse> response_;
    std::string lastHost_;
    std::string cacertFile_;
    std::string certFile_;
    std::string keyFile_;
};

} // namespace net

#endif // NET_HTTP_HTTPCLIENT_H
