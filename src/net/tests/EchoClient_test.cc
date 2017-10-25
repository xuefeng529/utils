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

char g_text[4*1024];

class EchoClient;
boost::ptr_vector<EchoClient> g_clients;
int g_current = 0;
base::AtomicInt32 g_numConns;

class EchoClient : boost::noncopyable
{
public:
	EchoClient(EventLoop* loop,
			   const InetAddress& listenAddr,
		       const std::string& id,
		       uint64_t heartbeat)
		: loop_(loop),
		  client_(loop, listenAddr, "EchoClient" + id, heartbeat),
          numBytes_(0)
	{
		client_.setConnectionCallback(
			boost::bind(&EchoClient::onConnection, this, _1));
		client_.setMessageCallback(
			boost::bind(&EchoClient::onMessage, this, _1, _2));
		client_.setWriteCompleteCallback(
			boost::bind(&EchoClient::onWriteComplete, this, _1));
		client_.setHearbeatCallback(
			boost::bind(&EchoClient::onHeartbeat, this, _1));
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
	void onConnection(const TcpConnectionPtr& conn)
	{
        LOG_DEBUG << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
		
		if (conn->connected())
		{		
            LOG_INFO << "the number of connections: " << g_numConns.incrementAndGet();
            conn->setContext(g_current);
            conn->send(g_text, sizeof(g_text));
            ++g_current;
            if (static_cast<size_t>(g_current) < g_clients.size())
            {
                g_clients[g_current].connect();
            }
            //conn->close();           
		}	
        else
        {
            LOG_INFO << "the number of connections: " << g_numConns.decrementAndGet();
        }
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		//LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";        
		std::string msg;
        buffer->retrieveAllAsString(&msg);
        assert(buffer->length() == 0);
        conn->send(msg);
	}

	void onWriteComplete(const net::TcpConnectionPtr& conn)
	{
       // LOG_INFO << "onWriteComplete [" << conn->name() << "]";       		
	}

	void onHeartbeat(const net::TcpConnectionPtr& conn)
	{
		LOG_INFO << "onHeartbeat";		
	}
    
	EventLoop* loop_;
	TcpClient client_;
    int64_t numBytes_;
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

        g_clients.reserve(n);
        for (int i = 0; i < n; ++i)
        {
            char buf[32];
            snprintf(buf, sizeof buf, "%d", i + 1);
            g_clients.push_back(new EchoClient(&loop, serverAddr, buf, 0));
        }

        g_clients[g_current].connect();
		loop.loop();
	}
	else
	{
		LOG_INFO << "Usage: " << argv[0] << " ip port";
	}
}
