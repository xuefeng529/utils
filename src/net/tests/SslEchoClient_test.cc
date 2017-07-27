#include "net/TcpClient.h"
#include "base/Thread.h"
#include "base/Atomic.h"
#include "base/Logging.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"
#include "net/SslContext.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <iostream>

#include <utility>
#include <stdio.h>
#include <unistd.h>

using namespace base;
using namespace net;

char g_text[3 * 1024];

class SslEchoClient;
boost::ptr_vector<SslEchoClient> clients;
int current = 0;
base::AtomicInt32 numConn;

class SslEchoClient : boost::noncopyable
{
public:
    SslEchoClient(EventLoop* loop,
        const InetAddress& listenAddr,
        const std::string& id,
        uint64_t heartbeat,
        SslContext* sslCtx)
        : loop_(loop),
        client_(loop, listenAddr, "SslEchoClient" + id, heartbeat, sslCtx)
    {
        client_.setConnectionCallback(
            boost::bind(&SslEchoClient::onConnection, this, _1));
        client_.setMessageCallback(
            boost::bind(&SslEchoClient::onMessage, this, _1, _2));
        client_.setWriteCompleteCallback(
            boost::bind(&SslEchoClient::onWriteComplete, this, _1));
        client_.setHearbeatCallback(
            boost::bind(&SslEchoClient::onHeartbeat, this, _1));
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

        if (conn->connected())
        {
            conn->send(g_text, sizeof(g_text));
            ++current;
            if (static_cast<size_t>(current) < clients.size())
            {
                clients[current].connect();
            }
        }
    }

    void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
    {
        LOG_INFO << conn->name() << ": " << buffer->length() << " bytes";
        std::string msg;
        buffer->retrieveAllAsString(&msg);
        LOG_INFO << conn->name() << ": " << msg;
        conn->send(msg);
    }

    void onWriteComplete(const net::TcpConnectionPtr& conn)
    {
        //LOG_INFO << "onWriteComplete[" << conn->name() << "]";
        //conn->send(g_text, sizeof(g_text));
    }

    void onHeartbeat(const net::TcpConnectionPtr& conn)
    {
        LOG_INFO << "onHeartbeat";
    }

    EventLoop* loop_;
    TcpClient client_;
};

int main(int argc, char* argv[])
{
    base::Logger::setLogLevel(base::Logger::INFO);
    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    if (argc < 8)
    {
        LOG_FATAL << "Usage: " << argv[0] << "ip port numclients cacert cert key passwd";
    }

    memset(g_text, 'c', sizeof(g_text));
    net::SslContext sslCtx;
    sslCtx.init(argv[4], argv[5], argv[6], argv[7]);
    EventLoop loop;
    InetAddress serverAddr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    int n = atoi(argv[3]);
    clients.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "%d", i + 1);
        clients.push_back(new SslEchoClient(&loop, serverAddr, buf, 0, &sslCtx));
    }
    clients[current].connect();
    loop.loop();
}
