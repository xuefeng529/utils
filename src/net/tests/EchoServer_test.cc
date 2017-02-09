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

#include <boost/bind.hpp>
#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

net::EventLoop* g_loop;

int numThreads = 0;
base::AtomicInt32 numConn;
const size_t kBufferLen = 4 * 1024;
char g_text[kBufferLen];

typedef boost::weak_ptr<net::TcpConnection> WeakTcpConnectionPtr;

base::ThreadPool threads;

void handleTask(const WeakTcpConnectionPtr& weakConn, const std::string& msg)
{
	net::TcpConnectionPtr conn(weakConn.lock());
	if (conn)
	{
		//LOG_DEBUG << "handleTask";
		conn->send(msg);
	}
}

class EchoServer
{
public:
	struct Entry
	{
		size_t numBytes;
		int cnt;
	};

	typedef boost::shared_ptr<Entry> EntryPtr;

	EchoServer(net::EventLoop* loop, const net::InetAddress& listenAddr, time_t readIdle)
		: loop_(loop),
		server_(loop, listenAddr, "EchoServer", readIdle)
	{
		server_.setConnectionCallback(
			boost::bind(&EchoServer::onConnection, this, _1));
		server_.setMessageCallback(
			boost::bind(&EchoServer::onMessage, this, _1, _2));
		server_.setWriteCompleteCallback(
			boost::bind(&EchoServer::onWriteComplete, this, _1));
		/*server_.setReadTimeoutCallback(
			boost::bind(&EchoServer::onReadTimeout, this, _1));*/
		server_.setThreadNum(numThreads);
	}

	void start()
	{
		server_.start();
	}

private:
	void onConnection(const net::TcpConnectionPtr& conn)
	{
		/*LOG_INFO << conn->peerAddress().toIpPort() << " -> "
			<< conn->localAddress().toIpPort() << " is "
			<< (conn->connected() ? "UP" : "DOWN");*/

		//LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
		if (!conn->connected())
		{
			LOG_INFO << "Connection number: " << numConn.decrementAndGet();
			/*Entry* entry = boost::any_cast<Entry>(conn->getMutableContext());
			LOG_ERROR << "recved the number of byte: " << entry->numBytes / 1024;
			if (entry->numBytes != 4 * 1024 * 10)
			{
			LOG_ERROR << "recved the number of byte error: ";
			}*/
		}
		else
		{
			LOG_INFO << "Connection number: " << numConn.incrementAndGet();
			//size_t numBytes = 0;
			//conn->setContext(numBytes);
			/*Entry entry = { 0, 0 };
			conn->setContext(entry);*/
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		//LOG_INFO << conn->name() << ": " << buffer->length();
		//size_t* numBytes = boost::any_cast<size_t>(conn->getMutableContext());
		//*numBytes += buffer->length();
		net::BufferPtr sendBuffer(new net::Buffer());
		sendBuffer->removeBuffer(buffer);
		assert(buffer->length() == 0);
		conn->send(sendBuffer);
		/*if (*numBytes == 4096)
		{
		conn->close();
		}*/
		
		/*Entry* entry = boost::any_cast<Entry>(conn->getMutableContext());
		if (entry->cnt < 10)
		{
			entry->numBytes += buffer->length();
			net::BufferPtr sendBuffer(new net::Buffer());
			sendBuffer->removeBuffer(buffer);
			conn->send(sendBuffer);
			entry->cnt++;
		}
		else
		{
			conn->shutdown();
		}*/
	}

	void onWriteComplete(const net::TcpConnectionPtr& conn)
	{
		//LOG_INFO << "onWriteComplete" << "[" << conn->name() << "]";
		//conn->send("hello\n");
	}

	/*void onReadTimeout(const net::TcpConnectionPtr& conn)
	{
	LOG_INFO << conn->name() << ": onReadTimeout";
	}*/

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
	//base::Logger::setTimeZone(shanghai);
	base::Logger::setLogLevel(base::Logger::INFO);
	//base::Logger::setOutput(asyncOutput);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();

	//threads.setMaxQueueSize(10000);
	//threads.start(4);

	memset(g_text, 's', sizeof(g_text));
	net::EventLoop loop;
	//g_loop = &loop;
	net::InetAddress listenAddr(static_cast<uint16_t>(atoi(argv[1])));
	EchoServer server(&loop, listenAddr, 0);
	server.start();
	loop.loop();
	//log.stop();
	std::cout << "done ." << std::endl;
}
