#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"
#include "net/http/HttpClient.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"
#include "net/SslContext.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <iostream>

#include <utility>
#include <stdio.h>
#include <unistd.h>

//std::string g_host;
//
//void sendRequest(net::HttpRequest* request)
//{
//	static int cnt = 0;
//	switch (cnt)
//	{
//	case 0:
//	{
//		request->setMethod(net::HttpRequest::kGet);
//		request->setPath("/find");
//		request->setQuery("a=1&b=2");
//		request->setCloseConnection(false);
//		break;
//	}
//	case 1:
//	{
//		request->setMethod(net::HttpRequest::kGet);
//		request->parseUrl("/find?a=1&b=2");
//		request->setBody("body test");
//		request->setCloseConnection(false);
//		break;
//	}
//	case 2:
//	{
//		request->setMethod(net::HttpRequest::kGet);
//		char url[128];
//		snprintf(url, sizeof(url), "http://%s/find?a=1&b=2", g_host.c_str());
//		request->parseUrl(url);
//		break;
//	}
//	}
//
//	cnt++;
//}
//
//void handleResponse(const net::HttpResponse& response)
//{
//	LOG_INFO << response.statusCode() << " " << response.statusMessage();
//	LOG_INFO << "body:" << response.body();
//}

int main(int argc, char* argv[])
{
	base::Logger::setLogLevel(base::Logger::INFO);
    bool keepalive = false;
    if (argc > 1)
    {
        keepalive = (atoi(argv[1]) != 0);
    }

	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
    LOG_INFO << "please input url or q exit!";
    net::SslContext sslCtx;
    std::string line;
    boost::scoped_ptr<net::HttpClient> httpClient(new net::HttpClient());
    if (argc == 5)
    {
        if (strcmp(argv[2], "\"\"") == 0)
        {
            sslCtx.init("", argv[3], argv[4], "");
        }
        else
        {
            sslCtx.init(argv[2], argv[3], argv[4], "");
        }   
        httpClient.reset(new net::HttpClient(&sslCtx));
    }

    if (argc == 6)
    {
        if (strcmp(argv[2], "\"\"") == 0)
        {
            sslCtx.init("", argv[3], argv[4], argv[5]);
        }
        else
        {
            sslCtx.init(argv[2], argv[3], argv[4], argv[5]);
        }
        httpClient.reset(new net::HttpClient(&sslCtx));
    }

    while (std::getline(std::cin, line) && line != "q")
    {
        net::HttpResponse response = httpClient->request(line, net::HttpRequest::kGet, keepalive);
        LOG_INFO << response.statusCode() << " " << response.statusMessage();
        LOG_INFO << "body:" << response.body();
    }
}
