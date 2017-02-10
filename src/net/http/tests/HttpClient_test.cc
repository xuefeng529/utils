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

net::EventLoop* g_loop;

void sendRequest(net::HttpRequest* request)
{
	static int cnt = 0;
	request->setMethod(net::HttpRequest::kGet);
	request->setPath("/find");
	request->setQuery("a=1&b=2");
	if (cnt < 1)
	{
		request->setCloseConnection(false);
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
		net::EventLoop loop;
		g_loop = &loop;
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
