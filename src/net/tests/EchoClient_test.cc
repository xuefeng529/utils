#include "net/TcpClient.h"
#include "base/Thread.h"
#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <iostream>

#include <utility>
#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

char g_text[4*1024];

int numThreads = 0;
class EchoClient;
boost::ptr_vector<EchoClient> clients;
int current = 0;
int numClosed = 0;

const char* url = "GET /UserCfgGet.aspx?a=6&b=1,600002 HTTP/1.0\r\n"
"Accept: */*\r\n"
"Connection: Keep - Alive\r\n"
"Host: 172.16.56.27:9898\r\n"
"User-Agent: ApacheBench/2.3\r\n\r\n";


class EchoClient : boost::noncopyable
{
public:
	EchoClient(EventLoop* loop,
			   const InetAddress& listenAddr,
		       const std::string& id,
		       uint64_t connectingExpire, 
		       uint64_t heartbeat)
		: loop_(loop),
		client_(loop, listenAddr, "EchoClient" + id, connectingExpire, heartbeat)
	{
		client_.setConnectionCallback(
			boost::bind(&EchoClient::onConnection, this, _1));
		client_.setMessageCallback(
			boost::bind(&EchoClient::onMessage, this, _1, _2));
		client_.setWriteCompleteCallback(
			boost::bind(&EchoClient::onWriteComplete, this, _1));
		client_.setConnectingExpireCallback(
			boost::bind(&EchoClient::onConnectingExpire, this));
		client_.setHearbeatCallback(
			boost::bind(&EchoClient::onHeartbeat, this, _1));
		//client_.enableRetry();
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
	void onConnection(const TcpConnectionPtr& conn)
	{
		/*LOG_DEBUG << conn->localAddress().toIpPort() << " -> "
			<< conn->peerAddress().toIpPort() << " is "
			<< (conn->connected() ? "UP" : "DOWN");*/

		LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
		if (conn->connected())
		{
			//clients[current].disconnect();
			++current;
			if (static_cast<size_t>(current) < clients.size())
			{
				clients[current].connect();
			}
			else
			{
				LOG_INFO << "connecting completed.";
			}
			int cnt = 1;
			conn->setContext(cnt);
			//conn->send("my very good time");
			//LOG_DEBUG << "*** connected " << current;
			//conn->send(static_cast<void*>(g_text), sizeof(g_text));
			conn->send(g_text);
		}
		else
		{
			numClosed++;
			LOG_INFO << "numClosed: " << numClosed;
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		std::string msg;
		buffer->retrieveAllAsString(&msg);
		conn->send(msg);
		LOG_INFO << conn->name() << ": " << msg.size() << " bytes";
		/*int cnt = boost::any_cast<int>(conn->getContext());
		cnt++;
		conn->setContext(cnt);*/
		/*if (cnt == 10)
		{
		conn->close();
		}*/
		
		/*net::BufferPtr sendBuffer(new net::Buffer());
		sendBuffer->removeBuffer(buffer);
		conn->send(sendBuffer);*/
	}

	void onWriteComplete(const net::TcpConnectionPtr& conn)
	{
		//LOG_INFO << "onWriteComplete";
		//conn->send(g_text);
	}

	void onConnectingExpire()
	{
		LOG_INFO << "onConnectingExpire";
		//client_.connect();
	}

	void onHeartbeat(const net::TcpConnectionPtr& conn)
	{
		//LOG_INFO << "onHeartbeat";
		conn->send("hearbeat");
	}

	EventLoop* loop_;
	TcpClient client_;
};

int main(int argc, char* argv[])
{
	base::Logger::setLogLevel(base::Logger::INFO);
	LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
	if (argc > 2)
	{
		memset(g_text, 'c', sizeof(g_text));

		EventLoop loop;
		InetAddress serverAddr(argv[1], static_cast<uint16_t>(atoi(argv[2])));

		int n = 1;
		if (argc > 3)
		{
			n = atoi(argv[3]);
		}

		clients.reserve(n);
		for (int i = 0; i < n; ++i)
		{
			char buf[32];
			snprintf(buf, sizeof buf, "%d", i + 1);
			clients.push_back(new EchoClient(&loop, serverAddr, buf, 30, 8));
			clients[i].connect();
		}

		clients[current].connect();
		loop.loop();
	}
	else
	{
		LOG_INFO << "Usage: " << argv[0] << " host_ip[current#]";
	}
}
