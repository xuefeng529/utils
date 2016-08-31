#include "base/Thread.h"
#include "base/Logging.h"
#include "base/AsyncLogging.h"
#include "base/TimeZone.h"
#include "base/ThreadPool.h"
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
int numConn = 0;
int numClosed = 0;

char g_text[8*1024];

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
		int msgCout;
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

		LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
		if (!conn->connected())
		{
			numClosed++;
			LOG_INFO << "numClosed: " << numClosed;
		}
		else
		{
			//int cnt = 0;
			//conn->setContext(cnt);
			//conn->send("my very good time");
		}
		//if (conn->connected())
		//{
		//	numConn++;
		//	if (numConn == 20000)
		//	{
		//		LOG_INFO << "Total conns: " << numConn;
		//	}
		//	//EntryPtr entry(new Entry());
		//	//entry->msgCout = 0;
		//	//conn->setContext(entry);
		//	//conn->send(static_cast<void*>(g_text), sizeof(g_text));
		//}
		//else
		//{
		//	LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
		//}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		//LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
		//buffer->retrieveAll();
		//std::string msg;
		//buffer->retrieveAllAsString(&msg);
		//if (msg == "quit\r\n")
		//{
		//	//conn->close();
		//	loop_->quit();
		//}
		//else
		//{
		//	conn->send(msg);
		//}
		net::BufferPtr sendBuffer(new net::Buffer());
		sendBuffer->removeBuffer(buffer);
		conn->send(sendBuffer);
		/*int cnt = boost::any_cast<int>(conn->getContext());
		cnt++;
		conn->setContext(cnt);
		if (cnt == 10)
		{
		conn->close();
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
	base::Logger::setTimeZone(shanghai);
	base::Logger::setLogLevel(base::Logger::INFO);
	//base::Logger::setOutput(asyncOutput);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();

	//threads.setMaxQueueSize(10000);
	//threads.start(4);

	memset(g_text, 's', sizeof(g_text));
	net::EventLoop loop;
	//g_loop = &loop;
	net::InetAddress listenAddr(static_cast<uint16_t>(atoi(argv[1])));
	EchoServer server(&loop, listenAddr, 180);
	server.start();
	loop.loop();
	//log.stop();
	std::cout << "done ." << std::endl;
}
