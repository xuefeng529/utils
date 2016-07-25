#include "base/Thread.h"
#include "base/Logging.h"
#include "base/AsyncLogging.h"
#include "base/TimeZone.h"
#include "base/StringUtil.h"
#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"

#include "plugins/http_lite/HttpContext.h"
#include "plugins/http_lite/HttpResponse.h"

#include <boost/bind.hpp>
#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

int numThreads = 0;

class HttpServer
{
public:
	typedef boost::shared_ptr<plugin::http::HttpContext> HttpContextPtr;
	typedef boost::weak_ptr<net::TcpConnection> WeakTcpConnectionPtr;

	HttpServer(net::EventLoop* loop, const net::InetAddress& listenAddr)
		: loop_(loop),
		server_(loop, listenAddr, "HttpServer")
	{
		server_.setConnectionCallback(
			boost::bind(&HttpServer::onConnection, this, _1));
		server_.setMessageCallback(
			boost::bind(&HttpServer::onMessage, this, _1, _2));
		server_.setThreadNum(numThreads);
	}

	void start()
	{
		server_.start();
	}

private:
	void onConnection(const net::TcpConnectionPtr& conn)
	{
		LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
		if (conn->connected())
		{
			WeakTcpConnectionPtr weakConn(conn);
			HttpContextPtr context(new plugin::http::HttpContext());
			context->setRequestCallback(boost::bind(&HttpServer::onHttpRequest, this, weakConn, _1));
			conn->setContext(context);
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		
		std::string msg;
		buffer->retrieveAllAsString(&msg);
		//LOG_INFO << conn->name() << " recv " << msg.size() << " bytes";
		//conn->send(msg);
		assert(!conn->getContext().empty());
		HttpContextPtr context = boost::any_cast<HttpContextPtr>(conn->getContext());
		context->parseRequest(msg.data(), msg.size());
	}

	void onHttpRequest(const WeakTcpConnectionPtr& weakConn, const plugin::http::HttpRequest& httpRequest)
	{
		LOG_INFO << httpRequest.methodString();
		LOG_INFO << httpRequest.path();
		LOG_INFO << httpRequest.query();
		const std::map<std::string, std::string>& headers = httpRequest.headers();
		std::map<std::string, std::string>::const_iterator it = headers.begin();
		for (; it != headers.end(); ++it)
		{
			LOG_INFO << it->first << ": " << it->second;
		}

		net::TcpConnectionPtr conn = weakConn.lock();
		if (conn)
		{
			plugin::http::HttpResponse resp(false);
			resp.setStatusCode(plugin::http::HttpResponse::k200Ok);
			resp.setStatusMessage("it is ok");
			resp.setBody("it is body", strlen("it is body"));
			base::Buffer buf;
			resp.appendToBuffer(&buf);
			conn->send(buf.retrieveAllAsString());
		}
	}

	net::EventLoop* loop_;
	net::TcpServer server_;
};

int kRollSize = 500 * 1000 * 1000;

base::AsyncLogging* g_asyncLog = NULL;

void asyncOutput(const char* msg, int len)
{
	g_asyncLog->append(msg, len);
}

int main(int argc, char* argv[])
{
	/*if (daemon(1, 0) < 0)
	{
	perror("daemon fatal");
	abort();
	}*/

	if (argc > 2)
	{
		numThreads = atoi(argv[2]);
	}

	char name[256];
	strncpy(name, argv[0], 256);
	//base::AsyncLogging log(::basename(name), kRollSize);
	//log.start();
	//g_asyncLog = &log;
	base::TimeZone shanghai("/usr/share/zoneinfo/Asia/Shanghai");
	base::Logger::setTimeZone(shanghai);
	base::Logger::setLogLevel(base::Logger::INFO);
	//base::Logger::setOutput(asyncOutput);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();

	net::EventLoop loop;
	net::InetAddress listenAddr(atoi(argv[1]));
	HttpServer server(&loop, listenAddr);
	server.start();

	//loop.loop();
	LOG_INFO << "done .";
}

