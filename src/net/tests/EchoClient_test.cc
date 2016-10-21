#include "net/TcpClient.h"
#include "base/Thread.h"
#include "base/Atomic.h"
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

std::string g_text(4096, 'a');

class EchoClient;
boost::ptr_vector<EchoClient> clients;
int current = 0;
base::AtomicInt32 numConn;
//base::AtomicInt32 numClosed;

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
			++current;
			if (static_cast<size_t>(current) < clients.size())
			{
				clients[current].connect();
			}
			LOG_INFO << "Connection number: " << numConn.incrementAndGet();
			size_t numBytes = 0;
			conn->setContext(numBytes);
			conn->send(g_text);
		}
		else
		{
			LOG_INFO << "Connection number: " << numConn.decrementAndGet();
			size_t numBytes = boost::any_cast<size_t>(conn->getContext());
			LOG_ERROR << "recved the number of byte: " << numBytes / 1024;
			if (numBytes != 4 * 1024 * 10)
			{
				LOG_ERROR << "recved the number of byte error: ";
			}
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
		
		size_t* numBytes = boost::any_cast<size_t>(conn->getMutableContext());
		*numBytes += buffer->length();
		std::string msg;
		buffer->retrieveAllAsString(&msg);
		conn->send(msg);
		
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
		/*static int count = 0;
		count++;
		LOG_INFO << "count: " << count;*/
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
		//memset(g_text, 'c', sizeof(g_text));
		//g_text[sizeof(g_text)-1] = 0;

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
			clients.push_back(new EchoClient(&loop, serverAddr, buf, 30, 0));
		}

		clients[current].connect();
		loop.loop();
	}
	else
	{
		LOG_INFO << "Usage: " << argv[0] << " host_ip[current#]";
	}
}
