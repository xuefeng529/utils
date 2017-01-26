#include "net/TcpClient.h"
#include "base/Thread.h"
#include "base/Atomic.h"
#include "base/Logging.h"
#include "base/CountDownLatch.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <iostream>

#include <utility>
#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

char g_text[4 * 1024];

class HttpRequest;
std::vector<HttpRequest*> clients;
int current;
int connLimit = 1;
base::AtomicInt32 connectedNum;
//base::AtomicInt32 disconnectedNum;

boost::scoped_ptr<base::CountDownLatch> theConnectedLatch;
boost::scoped_ptr<base::CountDownLatch> theDisconnectedLatch;

base::MutexLock theLock;

int respNum;
const char* url = 
"GET /UserInfoSet?uid=7121013685952264&did=&token=A0000059FAFDEB&mobile=0&appType=102&appVer=6.4.1&flag=31 HTTP/1.0\r\n"
"Accept: */*\r\n"
"Connection: close\r\n"
"Host: 172.16.40.249:9898\r\n"
"User-Agent: ApacheBench/2.3\r\n\r\n";

void sendRequest();
void resetClients();

class HttpRequest : boost::noncopyable
{
public:
	HttpRequest(EventLoop* loop,
		const InetAddress& listenAddr,
		const std::string& id,
		uint64_t connectingExpire,
		uint64_t heartbeat)
		: loop_(loop),
		client_(loop, listenAddr, "HttpRequest" + id, connectingExpire, heartbeat)
	{
		client_.setConnectionCallback(
			boost::bind(&HttpRequest::onConnection, this, _1));
		client_.setMessageCallback(
			boost::bind(&HttpRequest::onMessage, this, _1, _2));
		client_.setWriteCompleteCallback(
			boost::bind(&HttpRequest::onWriteComplete, this, _1));
		client_.setConnectingExpireCallback(
			boost::bind(&HttpRequest::onConnectingExpire, this));
		client_.setHearbeatCallback(
			boost::bind(&HttpRequest::onHeartbeat, this, _1));
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

	void send(const char* str)
	{
		client_.send(str);
	}

private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		/*LOG_DEBUG << conn->localAddress().toIpPort() << " -> "
		<< conn->peerAddress().toIpPort() << " is "
		<< (conn->connected() ? "UP" : "DOWN");*/

		//LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
		if (conn->connected())
		{
			conn->send(url);
			connectedNum.increment();

			{
				base::MutexLockGuard lock(theLock);
				++current;
			}

			if (static_cast<size_t>(current) < clients.size())
			{
				clients[current]->connect();
			}
			
			//theConnectedLatch->countDown();
		}
		else
		{
			connectedNum.decrement();
			//theDisconnectedLatch->countDown();
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		std::string msg;
		buffer->retrieveAllAsString(&msg);
		client_.disconnect();
		//LOG_INFO << "the number of response: " << ++respNum;
	}

	void onWriteComplete(const net::TcpConnectionPtr& conn)
	{
		//LOG_INFO << "onWriteComplete";
		//conn->send(g_text);
	}

	void onConnectingExpire()
	{
		LOG_INFO << "onConnectingExpire";
		client_.connect();
	}

	void onHeartbeat(const net::TcpConnectionPtr& conn)
	{
		LOG_INFO << "onHeartbeat";
		/*static int count = 0;
		count++;
		LOG_INFO << "count: " << count;*/
		//conn->send("hearbeat");
	}

	EventLoop* loop_;
	TcpClient client_;
};

void checkThreadFun()
{
	LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();

	bool sending = false;
	while (true)
	{
		int32_t num = connectedNum.get();
		LOG_INFO << "num: " << num;
		if (num == connLimit && !sending)
		{
			LOG_INFO << "sending";
			sending = true;
			for (int i = 0; i < connLimit; ++i)
			{
				clients[i]->send(url);
			}
		}
		else if (num == 0 && sending)
		{
			LOG_INFO << "reconnecting";

			{
				base::MutexLockGuard lock(theLock);
				current = 0;
			}
			sending = false;
			clients[current]->connect();
		}
		else
		{
			usleep(1000000);
		}
	}
}

int main(int argc, char* argv[])
{
	base::Logger::setLogLevel(base::Logger::INFO);
	LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
	if (argc > 2)
	{
		if (argc > 3)
		{
			connLimit = atoi(argv[3]);
		}

		//theConnectedLatch.reset(new base::CountDownLatch(connLimit));
		//theDisconnectedLatch.reset(new base::CountDownLatch(connLimit));

		//base::Thread reconnectThread(boost::bind(reconnectThreadFun));
		//reconnectThread.start();

		EventLoop loop;
		InetAddress serverAddr(argv[1], static_cast<uint16_t>(atoi(argv[2])));

		clients.reserve(connLimit);
		for (int i = 0; i < connLimit; ++i)
		{
			char buf[32];
			snprintf(buf, sizeof buf, "%d", i + 1);
			clients.push_back(new HttpRequest(&loop, serverAddr, buf, 60, 0));
		}

		current = 0;
		clients[current]->connect();

		//base::Thread checkThread(boost::bind(checkThreadFun));
		//checkThread.start();

		loop.loop();
	}
	else
	{
		LOG_INFO << "Usage: " << argv[0] << " host_ip[current#]";
	}
}
