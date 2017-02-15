#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"

#include "net/http/HttpClient.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <iostream>

#include <utility>
#include <stdio.h>
#include <unistd.h>

std::string g_host;

void sendRequest(net::HttpRequest* request)
{
	static int cnt = 0;
	switch (cnt)
	{
	case 0:
	{
		request->setMethod(net::HttpRequest::kGet);
		request->setPath("/find");
		request->setQuery("a=1&b=2");
		request->setCloseConnection(false);
		break;
	}
	case 1:
	{
		request->setMethod(net::HttpRequest::kGet);
		request->parseUrl("/find?a=1&b=2");
		request->setCloseConnection(false);
		break;
	}
	case 2:
	{
		request->setMethod(net::HttpRequest::kGet);
		char url[128];
		snprintf(url, sizeof(url), "http://%s/find?a=1&b=2", g_host.c_str());
		request->parseUrl(url);
		break;
	}
	}

	cnt++;
}

void handleResponse(const net::HttpResponse& response)
{
	LOG_INFO << response.statusCode() << " " << response.statusMessage();
	LOG_INFO << "body:" << response.body();
}

int main(int argc, char* argv[])
{
	base::Logger::setLogLevel(base::Logger::DEBUG);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
	if (argc > 2)
	{
		net::InetAddress addr(static_cast<uint16_t>(atoi(argv[2])));
		net::InetAddress::resolve(argv[1], &addr);
		g_host = addr.toIpPort();
		net::EventLoop loop;
		net::HttpClient httpClient(&loop, argv[1], static_cast<uint16_t>(atoi(argv[2])));
		httpClient.setSendRequestCallback(boost::bind(sendRequest, _1));
		httpClient.setResponseCallback(boost::bind(handleResponse, _1));
		httpClient.connect();
		loop.loop();
		LOG_INFO << "loop done.";
	}
	else
	{
		LOG_INFO << "Usage: " << argv[0] << " host_ip[current#]";
	}
}
