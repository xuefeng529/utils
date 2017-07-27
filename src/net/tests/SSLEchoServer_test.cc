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
#include "net/SslContext.h"

#include <boost/bind.hpp>
#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

int numThreads = 0;
base::AtomicInt32 numConn;

class SslEchoServer
{
public:
    SslEchoServer(net::EventLoop* loop, const net::InetAddress& listenAddr, time_t readIdle, net::SslContext* sslCtx)
        : loop_(loop),
        server_(loop, listenAddr, "SslEchoServer", readIdle, sslCtx)
    {
        server_.setConnectionCallback(
            boost::bind(&SslEchoServer::onConnection, this, _1));
        server_.setMessageCallback(
            boost::bind(&SslEchoServer::onMessage, this, _1, _2));
        server_.setWriteCompleteCallback(
            boost::bind(&SslEchoServer::onWriteComplete, this, _1));
        server_.setThreadNum(numThreads);
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const net::TcpConnectionPtr& conn)
    {
        if (!conn->connected())
        {
            LOG_INFO << "Connection number: " << numConn.decrementAndGet();            
        }
        else
        {
            LOG_INFO << "Connection number: " << numConn.incrementAndGet();           
        }
    }

    void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
    {
        //LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
        net::BufferPtr sendBuffer(new net::Buffer());
        sendBuffer->removeBuffer(buffer);
        assert(buffer->length() == 0);
        conn->send(sendBuffer);
    }

    void onWriteComplete(const net::TcpConnectionPtr& conn)
    {
        //LOG_INFO << "onWriteComplete" << "[" << conn->name() << "]";
    }

private:
    net::EventLoop* loop_;
    net::TcpServer server_;
};

int main(int argc, char* argv[])
{
    if (argc < 6)
    {
        LOG_FATAL << "Usage: " << argv[0] << " port num cacert cert key";
    }

    numThreads = atoi(argv[2]);
    char name[256];
    strncpy(name, argv[0], 256);
    base::Logger::setLogLevel(base::Logger::INFO); 
    LOG_INFO << "pid = " << getpid() << ", tid = " << base::CurrentThread::tid();
    net::SslContext sslCtx;
    sslCtx.init(argv[3], argv[4], argv[5], "");
    net::EventLoop loop;
    net::InetAddress listenAddr(static_cast<uint16_t>(atoi(argv[1])));
    SslEchoServer server(&loop, listenAddr, 10, &sslCtx);
    server.start();
    loop.loop();
    LOG_INFO << "done .";
    return 0;
}
