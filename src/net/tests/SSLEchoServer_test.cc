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

int numThreads = 0;

class SSLEchoServer
{
public:
    SSLEchoServer(net::EventLoop* loop, const net::InetAddress& listenAddr, time_t readIdle)
        : loop_(loop),
        server_(loop, listenAddr, "SSLEchoServer", readIdle)
    {
        server_.setConnectionCallback(
            boost::bind(&SSLEchoServer::onConnection, this, _1));
        server_.setMessageCallback(
            boost::bind(&SSLEchoServer::onMessage, this, _1, _2));
        server_.setWriteCompleteCallback(
            boost::bind(&SSLEchoServer::onWriteComplete, this, _1));
        server_.setThreadNum(numThreads);
    }

    void enableSSL(const std::string& cacertFile, const std::string& certFile, const std::string& privateKeyFile)
    {
        server_.enableSSL(cacertFile, certFile, privateKeyFile);
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const net::TcpConnectionPtr& conn)
    {
        LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
    }

    void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
    {
        LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
        std::string msg;
        buffer->retrieveAllAsString(&msg);
        LOG_INFO << msg;
        assert(buffer->length() == 0);
        conn->send(msg);
    }

    void onWriteComplete(const net::TcpConnectionPtr& conn)
    {
        LOG_INFO << "onWriteComplete" << "[" << conn->name() << "]";
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
    net::EventLoop loop;
    net::InetAddress listenAddr(static_cast<uint16_t>(atoi(argv[1])));
    SSLEchoServer server(&loop, listenAddr, 0);
    server.enableSSL(argv[3], argv[4], argv[5]);
    server.start();
    loop.loop();
    std::cout << "done ." << std::endl;
    return 0;
}
