#include "net/TcpClient.h"
#include "base/Thread.h"
#include "base/Atomic.h"
#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"
#include "net/LengthHeaderCodec.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <iostream>

#include <utility>
#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

struct Package
{
	int16_t cmd;
	int32_t flag;
	char name[32];
	int64_t value;
};

char g_text[2 * 4096];

class CodecClient;
boost::ptr_vector<CodecClient> clients;
int current = 0;
base::AtomicInt32 numConn;

class CodecClient : boost::noncopyable
{
public:
	CodecClient(EventLoop* loop,
		        const InetAddress& listenAddr,
		        const std::string& id,
		        uint64_t heartbeat)
		: loop_(loop),
		  client_(loop, listenAddr, "EchoClient" + id, heartbeat),
		  codec_(boost::bind(&CodecClient::onMessage, this, _1, _2))
	{
		client_.setConnectionCallback(
			boost::bind(&CodecClient::onConnection, this, _1));
		client_.setMessageCallback(
			boost::bind(&net::LengthHeaderCodec::onMessage, &codec_, _1, _2));
		client_.setWriteCompleteCallback(
			boost::bind(&CodecClient::onWriteComplete, this, _1));
		client_.setHearbeatCallback(
			boost::bind(&CodecClient::onHeartbeat, this, _1));
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
		LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
		if (conn->connected())
		{
			LOG_INFO << "Connection number: " << numConn.incrementAndGet();
			Package pack;
			pack.cmd = 10;
			pack.flag = 1;
			std::string s("name");
			strncpy(pack.name, s.c_str(), s.size());
			pack.value = 123;
			codec_.send(conn, reinterpret_cast<char*>(&pack), sizeof(pack));
			++current;
			if (static_cast<size_t>(current) < clients.size())
			{
				clients[current].connect();
			}
		}
		else
		{
			LOG_INFO << "Connection number: " << numConn.decrementAndGet();
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
		Package pack;
		buffer->retrieveAsBytes(reinterpret_cast<char*>(&pack), sizeof(pack));

		LOG_INFO << "cmd: " << pack.cmd << " flag: " << pack.flag << " name: " << pack.name << " value: " << pack.value;
	}

	void onWriteComplete(const net::TcpConnectionPtr& conn)
	{
		LOG_INFO << "onWriteComplete";
	}

	void onHeartbeat(const net::TcpConnectionPtr& conn)
	{
		LOG_INFO << "onHeartbeat";
	}

	EventLoop* loop_;
	TcpClient client_;
	net::LengthHeaderCodec codec_;
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
			clients.push_back(new CodecClient(&loop, serverAddr, buf, 0));
		}

		clients[current].connect();
		loop.loop();
	}
	else
	{
		LOG_INFO << "Usage: " << argv[0] << " host_ip[current#]";
	}
}
