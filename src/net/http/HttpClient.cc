#include "net/http/HttpClient.h"
#include "net/http/HttpContext.h"
#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include "base/CountDownLatch.h"
#include "base/StringUtil.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <strings.h>

namespace net
{

HttpClient::HttpClient()
    : loopThread_(new EventLoopThread()),
      loop_(loopThread_->startLoop())
{
    assert(loop_ != NULL);
}

HttpClient::~HttpClient()
{
}

void HttpClient::enableSSL(const std::string& cacertFile, const std::string& certFile, const std::string& keyFile)
{
    cacertFile_ = cacertFile;
    certFile_ = certFile;
    keyFile_ = keyFile;
}

HttpResponse HttpClient::request(const std::string& url, HttpRequest::Method method, bool keepalive)
{
    struct http_parser_url u;
    if (http_parser_parse_url(url.c_str(), url.size(), 0, &u) != 0)
    {
        LOG_ERROR << "http_parser_parse_url error: " << url;
        return HttpResponse();
    }

    std::string schema;
    std::string host;
    uint16_t port;
    std::string path;
    std::string query;
    if (u.field_set & (1 << UF_SCHEMA))
    {
        schema = url.substr(u.field_data[UF_SCHEMA].off, u.field_data[UF_SCHEMA].len);
    }

    if (u.field_set & (1 << UF_HOST))
    {
        host = url.substr(u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
    }

    if (u.field_set & (1 << UF_PORT))
    {
        port = u.port;
    }
    else
    {
        if (strcasecmp(schema.c_str(), "https") == 0)
        {
            port = 443;
        }
        else
        {
            port = 80;
        }
    }

    if (u.field_set & (1 << UF_PATH))
    {
        path = url.substr(u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);      
    }

    if (u.field_set & (1 << UF_QUERY))
    {
        query = url.substr(u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);       
    }
    
    net::InetAddress serverAddr(port);
    if (!net::InetAddress::resolve(host.c_str(), &serverAddr))
    {
        LOG_ERROR << host;
        return HttpResponse();
    }

    host = serverAddr.toIpPort();
    HttpRequest request;
    request.setMethod(method);
    request.setPath(path);
    request.setQuery(query);
    request.addHeader("Host", host);
    request.setCloseConnection(!keepalive);
    if (host != lastHost_ || !client_ || !client_->isConnected() || !keepalive)
    {
        client_.reset(new TcpClient(loop_, serverAddr, "HttpClient"));          
        client_->setConnectionCallback(
            boost::bind(&HttpClient::handleConnection, this, _1, request));
        client_->setMessageCallback(
            boost::bind(&HttpClient::handleMessage, this, _1, _2));
        if (!cacertFile_.empty() && !keyFile_.empty() && !certFile_.empty())
        {
            client_->enableSSL(cacertFile_, certFile_, keyFile_);
        }
        client_->connect();
    }
    else
    {
        BufferPtr buffer(new Buffer());
        request.appendToBuffer(buffer.get());
        client_->send(buffer);
    }  
    lastHost_ = host;
    requestLatch_.reset(new base::CountDownLatch(1));
    requestLatch_->wait();
    return *response_;
}

void HttpClient::handleConnection(const TcpConnectionPtr& conn, const HttpRequest& request)
{
    if (conn->connected())
    {
        HttpContext context(conn, HttpContext::kResponse);
        context.setResponseCallback(boost::bind(&HttpClient::handleResponse, this, _1, _2));
        conn->setContext(context);
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
        client_.reset();
	}
}

void HttpClient::handleResponse(const TcpConnectionPtr& conn, const HttpResponse& response)
{
    response_.reset(new HttpResponse(response));
    requestLatch_->countDown();
}

} // namespace net
