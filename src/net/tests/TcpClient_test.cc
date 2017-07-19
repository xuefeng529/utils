#include "net/TcpClient.h"
#include "base/Thread.h"
#include "base/Atomic.h"
#include "base/Logging.h"
#include "base/StringUtil.h"
#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <string>

#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

class MyClient;
typedef boost::shared_ptr<MyClient> MyClientPtr;
typedef boost::weak_ptr<net::TcpConnection> TcpConnectionWeakPtr;
std::vector<MyClientPtr> g_clients;
std::vector<TcpConnectionWeakPtr> g_conns;

int current = 0;

class MyClient : boost::noncopyable
{
public:
	MyClient(net::EventLoop* loop,
		     const net::InetAddress& listenAddr,
			 const std::string& name,
			 int index)
			 : loop_(loop),
			   client_(loop, listenAddr, name, 0),
			   index_(index)
	{
		client_.setConnectionCallback(
			boost::bind(&MyClient::onConnection, this, _1));
		client_.setMessageCallback(
			boost::bind(&MyClient::onMessage, this, _1, _2));
		client_.enableRetry();
	}

	void connect()
	{
		client_.connect();
	}

	void disconnect()
	{
		client_.disconnect();
	}

private:
	void onConnection(const net::TcpConnectionPtr& conn)
	{
		LOG_DEBUG << conn->localAddress().toIpPort() << " -> "
			<< conn->peerAddress().toIpPort() << " is "
			<< (conn->connected() ? "UP" : "DOWN");

		if (conn->connected())
		{
			g_conns[index_] = conn;
			++current;
			if (static_cast<size_t>(current) < g_clients.size())
			{
				g_clients[current]->connect();
			}
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		std::string msg;
		buffer->retrieveAllAsString(&msg);
		LOG_INFO << index_ << ": " << msg;
	}

	net::EventLoop* loop_;
	net::TcpClient client_;
	int index_;
};

int main(int argc, char* argv[])
{
	base::Logger::setLogLevel(base::Logger::INFO);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
	if (argc > 2)
	{
		net::EventLoopThread cliLoopThread;
		net::EventLoop* cliLoop = cliLoopThread.startLoop();
		net::InetAddress serverAddr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
		int n = 1;
		if (argc > 3)
		{
			n = atoi(argv[3]);
		}

		g_clients.resize(n);
		for (int i = 0; i < n; ++i)
		{
			char buf[32];
			snprintf(buf, sizeof(buf), "MyClient_%d", i + 1);
			g_clients.push_back(MyClientPtr(new MyClient(cliLoop, serverAddr, buf, i)));
		}

		g_clients[current]->connect();

		std::string input;
		while (std::getline(std::cin, input))
		{
			std::vector<std::string> items;
			base::StringUtil::split(input, " ", &items);
			if (items.size() > 2)
			{
				net::TcpConnectionPtr conn(g_conns[base::StringUtil::strToUInt32(items[0])].lock());
				if (conn)
				{
					conn->send(items[1]);
				}
			}
		}
	}
	else
	{
		LOG_INFO << "Usage: " << argv[0] << " host_ip[current#]";
	}
}
