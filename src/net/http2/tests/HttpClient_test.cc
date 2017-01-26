#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"

#include "net/http2/HttpClient.h"
#include "net/http2/HttpResponse.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <iostream>

#include <utility>
#include <stdio.h>
#include <unistd.h>

void sendRequest(net::HttpRequest* request)
{
	request->setMethod(net::HttpRequest::kGet);
	request->setPath("/UserCfgGet.aspx");
	request->setQuery("?a=u01181519");
}

void handleResponse(const net::HttpResponse& response)
{
	LOG_INFO << response.statusCode() << " " << response.statusMessage();
	LOG_INFO << response.body();
}

int main(int argc, char* argv[])
{
	base::Logger::setLogLevel(base::Logger::INFO);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
	if (argc > 2)
	{
		net::EventLoop loop;
		net::InetAddress serverAddr(argv[1], static_cast<uint16_t>(atoi(argv[2])));

		net::HttpClient httpClient(&loop, serverAddr, "HttpClient_test", 60);
		httpClient.setSendRequestCallback(boost::bind(sendRequest, _1));
		httpClient.setResponseCallback(boost::bind(handleResponse, _1));
		httpClient.connect();

		/*clients.reserve(connLimit);
		for (int i = 0; i < connLimit; ++i)
		{
		char buf[32];
		snprintf(buf, sizeof buf, "%d", i + 1);
		clients.push_back(new net::HttpRequest(&loop, serverAddr, buf, 60, 0));
		}

		current = 0;
		clients[current]->connect();*/

		//base::Thread checkThread(boost::bind(checkThreadFun));
		//checkThread.start();

		loop.loop();
	}
	else
	{
		LOG_INFO << "Usage: " << argv[0] << " host_ip[current#]";
	}
}
