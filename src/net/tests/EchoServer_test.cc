#include "base/Thread.h"
#include "base/Logging.h"
#include "base/AsyncLogging.h"
#include "base/TimeZone.h"
#include "base/ThreadPool.h"
#include "base/Atomic.h"
#include "base/daemon.h"
#include "base/ProcessInfo.h"
#include "base/StringUtil.h"
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

int g_numIoThreads = 0;
base::AtomicInt64 g_numConns;

typedef boost::weak_ptr<net::TcpConnection> WeakTcpConnectionPtr;

boost::scoped_ptr<base::ThreadPool> theTaskHandler;

void handleTask(const WeakTcpConnectionPtr& weakConn, net::BufferPtr& msg)
{
	net::TcpConnectionPtr connGuard(weakConn.lock());
    if (connGuard)
	{
        //LOG_INFO << "handle sending task: " << msg->length() << " bytes [" << connGuard->name() << "]";
        connGuard->send(msg);
	}
}

class EchoServer
{
public:
    typedef struct Context 
    {
        std::string connName;
        size_t totalBytes;
        size_t numWriteComplte;
    } Context;

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
        server_.setThreadNum(g_numIoThreads);
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
		if (conn->connected())
		{
            LOG_INFO << "the number of connections: " << g_numConns.incrementAndGet();
            /* char buf[64];
             snprintf(buf, sizeof(buf), "conn_%"PRId32, connId_.incrementAndGet());
             Context ctx = { buf, 0, 0 };
             conn->setContext(ctx);   */
		}
		else
		{                        
            LOG_INFO << "the number of connections: " << g_numConns.decrementAndGet();
            /*const Context& ctx = boost::any_cast<Context>(conn->getContext());
            LOG_INFO << "total bytes: " << ctx.totalBytes << " write complete: " << ctx.numWriteComplte
            << " [" << ctx.connName << "]";*/
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{		
        LOG_INFO << "recv bytes: " << buffer->length() << "[" << conn->name() << "]";
        //buffer->retrieveAll();
        //Context* ctx = boost::any_cast<Context>(conn->getMutableContext());
        //ctx->totalBytes += buffer->length();
        ///*LOG_INFO << "total bytes: " << ctx->totalBytes << " write complete: " << ctx->numWriteComplte
        //    << " [" << ctx->connName << "]";*/
        net::BufferPtr sendBuffer(new net::Buffer());
        sendBuffer->removeBuffer(buffer);
        conn->send(sendBuffer);
        //assert(buffer->length() == 0);
        //if (theTaskHandler)
        //{
        //    theTaskHandler->run(boost::bind(handleTask, conn, sendBuffer));
        //}
        //else
        //{    
        //    LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
        //    conn->send(sendBuffer);
        //}				
	}

	void onWriteComplete(const net::TcpConnectionPtr& conn)
	{
        //LOG_INFO << "onWriteComplete " << "[" << conn->name() << "]";
        //Context* ctx = boost::any_cast<Context>(conn->getMutableContext());
        //ctx->numWriteComplte += 1;
        /*LOG_INFO << "total bytes: " << ctx->totalBytes << " write complete: " << ctx->numWriteComplte
            << " [" << ctx->connName << "]";*/
	}

    base::AtomicInt32 connId_;
	net::EventLoop* loop_;
	net::TcpServer server_;
};

const int kRollSize = 1000 * 1000 * 1000;
base::AsyncLogging* g_asyncLog = NULL;
void asyncOutput(const char* msg, int len)
{
	g_asyncLog->append(msg, len);
}

int main(int argc, char* argv[])
{
	if (argc > 2)
	{
        g_numIoThreads = atoi(argv[2]);
	}

    if (argc > 3)
    {
        theTaskHandler.reset(new base::ThreadPool());
        theTaskHandler->start(atoi(argv[3]));
    }

	char name[256];
	strncpy(name, argv[0], 256);
    base::Logger::setLogLevel(base::Logger::INFO);
    LOG_INFO << base::ProcessInfo::exePath() << " " << ::basename(name);
    base::AsyncLogging log(base::StringUtil::extractDirname(base::ProcessInfo::exePath()), ::basename(name), kRollSize);
    g_asyncLog = &log;
	//log.start();
	//base::Logger::setOutput(asyncOutput);
	LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
	net::EventLoop loop;	
	net::InetAddress listenAddr(static_cast<uint16_t>(atoi(argv[1])));
	EchoServer server(&loop, listenAddr, 0);
	server.start();
	loop.loop();
	log.stop();
    LOG_INFO << "done .";
}
