#include "base/Thread.h"
#include "base/Logging.h"
#include "base/AsyncLogging.h"
#include "base/TimeZone.h"
#include "base/ThreadPool.h"
#include "base/Atomic.h"
#include "base/daemon.h"
#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"
#include "net/LengthHeaderCodec.h"

#include <boost/bind.hpp>
#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

int numThreads = 0;
base::AtomicInt32 numConn;

struct Package
{
	int16_t cmd;
	int32_t flag;
	char name[32];
	int64_t value;
};

class CodecServer
{
public:
	CodecServer(net::EventLoop* loop, const net::InetAddress& listenAddr, time_t readIdle)
		: loop_(loop),
		  server_(loop, listenAddr, "CodecServer", readIdle),
		  codec_(boost::bind(&CodecServer::onMessage, this, _1, _2))
	{
		server_.setConnectionCallback(
			boost::bind(&CodecServer::onConnection, this, _1));
		server_.setMessageCallback(
			boost::bind(&net::LengthHeaderCodec::onMessage, &codec_, _1, _2));
		server_.setWriteCompleteCallback(
			boost::bind(&CodecServer::onWriteComplete, this, _1));
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
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
		Package pack;
		buffer->retrieveAsBytes(reinterpret_cast<char*>(&pack), sizeof(pack));

		LOG_INFO << "cmd: " << pack.cmd << " flag: " << pack.flag << " name: " << pack.name << " value: " << pack.value;
		codec_.send(conn, reinterpret_cast<char*>(&pack), sizeof(pack));
		conn->close();
	}

	void onWriteComplete(const net::TcpConnectionPtr& conn)
	{
		LOG_INFO << "onWriteComplete" << "[" << conn->name() << "]";
	}

	void onReadTimeout(const net::TcpConnectionPtr& conn)
	{
		LOG_INFO << conn->name() << ": onReadTimeout";
	}

private:
	net::EventLoop* loop_;
	net::TcpServer server_;
	net::LengthHeaderCodec codec_;
};

int main(int argc, char* argv[])
{
	if (argc > 2)
	{
		numThreads = atoi(argv[2]);
	}

	char name[256];
	strncpy(name, argv[0], 256);
	base::Logger::setLogLevel(base::Logger::INFO);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
	net::EventLoop loop;
	net::InetAddress listenAddr(static_cast<uint16_t>(atoi(argv[1])));
	CodecServer server(&loop, listenAddr, 0);
	server.start();
	loop.loop();
	std::cout << "done ." << std::endl;
	return 0;
}
