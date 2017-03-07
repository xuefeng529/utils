#include "net/http/http_parser.h"
#include "net/TcpClient.h"
#include "base/Thread.h"
#include "base/Logging.h"
#include "base/CountDownLatch.h"
#include "net/EventLoop.h"
#include "net/EventLoopThread.h"
#include "net/InetAddress.h"
#include "net/Buffer.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <string>
#include <vector>

#include <utility>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"

struct Context
{
	std::string name;
	std::string buffer;
	size_t offset;
	Context()
		: offset(0)
	{}
};

int onMessageBegin(http_parser* parser)
{
	Context* ctx = static_cast<Context*>(parser->data);
	printf("\n***MESSAGE BEGIN*** %s\n", ctx->name.c_str());
	return 0;
}

int onHeadersComplete(http_parser* parser)
{
	Context* ctx = static_cast<Context*>(parser->data);
	printf("\n***HEADERS COMPLETE*** %s\n", ctx->name.c_str());
	return 0;
}

int onMessageComplete(http_parser* parser)
{
	Context* ctx = static_cast<Context*>(parser->data);
	printf("\n***MESSAGE COMPLETE*** %s\n", ctx->name.c_str());
	printf("%s\n", ctx->buffer.c_str());
	ctx->buffer.clear();
	ctx->offset = 0;
	return 0;
}

int onUrl(http_parser* parser, const char* at, size_t length)
{
	Context* ctx = static_cast<Context*>(parser->data);
	printf("%s Url: %.*s\n", ctx->name.c_str(), (int)length, at);
	return 0;
}

int onHeaderField(http_parser* parser, const char* at, size_t length)
{
	Context* ctx = static_cast<Context*>(parser->data);
	printf("%s Header field: %.*s\n", ctx->name.c_str(), (int)length, at);
	return 0;
}

int onHeaderValue(http_parser* parser, const char* at, size_t length)
{
	Context* ctx = static_cast<Context*>(parser->data);
	printf("%s Header value: %.*s\n", ctx->name.c_str(), (int)length, at);
	return 0;
}

int onBody(http_parser* parser, const char* at, size_t length)
{
	Context* ctx = static_cast<Context*>(parser->data);
	printf("%s Body: %.*s\n", ctx->name.c_str(), (int)length, at);
	return 0;
}

void test1()
{
	http_parser_settings settings;
	memset(&settings, 0, sizeof(settings));
	settings.on_message_begin = onMessageBegin;
	settings.on_url = onUrl;
	settings.on_header_field = onHeaderField;
	settings.on_header_value = onHeaderValue;
	settings.on_headers_complete = onHeadersComplete;
	settings.on_body = onBody;
	settings.on_message_complete = onMessageComplete;

	http_parser parser;
	http_parser_init(&parser, HTTP_REQUEST);
	Context ctx;
	ctx.name = "test1";
	parser.data = &ctx;

	for (size_t i = 0; i < 1; i++)
	{
		std::string url1 = "GET /";
		std::string url2 = "test1.aspx?a=1&b=2 HTTP/1.0\r\n";
		std::vector<std::string> headers;
		headers.push_back("Ho");
		headers.push_back("st: localhost: 80\r\n");
		headers.push_back("Connection: Keep-Alive\r\n");
		
		std::string body1 = "body1";
		std::string body2 = "body2";
		headers.push_back("Con");
		headers.push_back("tent-");
		char buf[64];
		snprintf(buf, sizeof(buf), "Length: %zd\r\n", body1.size() + body2.size());
		headers.push_back(buf);

		headers.push_back("Cont");
		headers.push_back("ent-Type: text/");
		headers.push_back("plain\r\n\r\n");

		size_t len = url1.size();
		ctx.buffer.append(url1);
		const char* begin = ctx.buffer.data() + ctx.offset;
		ctx.offset += len;
		size_t nparsed = http_parser_execute(&parser, &settings, begin, len);
		if (nparsed != len)
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			ctx.buffer.clear();
			ctx.offset = 0;
			return;
		}

		len = url2.size();
		ctx.buffer.append(url2);
		begin = ctx.buffer.data() + ctx.offset;
		ctx.offset += len;
		nparsed = http_parser_execute(&parser, &settings, begin, len);
		if (nparsed != len)
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			ctx.buffer.clear();
			ctx.offset = 0;
			return;
		}

		for (size_t i = 0; i < headers.size(); i++)
		{
			std::string header = headers[i];
			ctx.buffer.append(header);
			len = header.size();
			begin = ctx.buffer.data() + ctx.offset;
			ctx.offset += len;
			nparsed = http_parser_execute(&parser, &settings, begin, len);
			if (nparsed != len)
			{
				fprintf(stderr,
					"Error: %s (%s)\n",
					http_errno_description(HTTP_PARSER_ERRNO(&parser)),
					http_errno_name(HTTP_PARSER_ERRNO(&parser)));
				ctx.buffer.clear();
				ctx.offset = 0;
				return;
			}
		}

		len = body1.size();
		ctx.buffer.append(body1);
		begin = ctx.buffer.data() + ctx.offset;
		ctx.offset += len;
		nparsed = http_parser_execute(&parser, &settings, begin, len);
		if (nparsed != len)
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			ctx.buffer.clear();
			ctx.offset = 0;
			return;
		}

		len = body2.size();
		ctx.buffer.append(body1);
		begin = ctx.buffer.data() + ctx.offset;
		ctx.offset += len;
		nparsed = http_parser_execute(&parser, &settings, begin, len);
		if (nparsed != len)
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			ctx.buffer.clear();
			ctx.offset = 0;
			return;
		}
	}
}

void test2()
{
	http_parser_settings settings;
	memset(&settings, 0, sizeof(settings));
	settings.on_message_begin = onMessageBegin;
	settings.on_url = onUrl;
	settings.on_header_field = onHeaderField;
	settings.on_header_value = onHeaderValue;
	settings.on_headers_complete = onHeadersComplete;
	settings.on_body = onBody;
	settings.on_message_complete = onMessageComplete;

	http_parser parser;
	http_parser_init(&parser, HTTP_REQUEST);
	Context ctx;
	ctx.name = "test2";
	parser.data = &ctx;

	for (size_t i = 0; i < 1; i++)
	{
		std::string url1 = "GET /";
		std::string url2 = "test2.aspx?a=1&b=2 HTTP/1.0\r\n";
		std::vector<std::string> headers;
		headers.push_back("Ho");
		headers.push_back("st: localhost: 80\r\n");
		headers.push_back("Connection: Keep-Alive\r\n");

		std::string body1 = "body1";
		std::string body2 = "body2";
		headers.push_back("Con");
		headers.push_back("tent-");
		char buf[64];
		snprintf(buf, sizeof(buf), "Length: %zd\r\n", body1.size() + body2.size());
		headers.push_back(buf);

		headers.push_back("Cont");
		headers.push_back("ent-Type: text/");
		headers.push_back("plain\r\n\r\n");

		size_t nparsed = http_parser_execute(&parser, &settings, url1.data(), url1.size());
		if (nparsed != url1.size())
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			return;
		}

		nparsed = http_parser_execute(&parser, &settings, url2.data(), url2.size());
		if (nparsed != url2.size())
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			return;
		}

		for (size_t i = 0; i < headers.size(); i++)
		{
			std::string header = headers[i];
			nparsed = http_parser_execute(&parser, &settings, header.data(), header.size());
			if (nparsed != header.size())
			{
				fprintf(stderr,
					"Error: %s (%s)\n",
					http_errno_description(HTTP_PARSER_ERRNO(&parser)),
					http_errno_name(HTTP_PARSER_ERRNO(&parser)));
				return;
			}
		}

		nparsed = http_parser_execute(&parser, &settings, body1.data(), body1.size());
		if (nparsed != body1.size())
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			return;
		}

		nparsed = http_parser_execute(&parser, &settings, body2.data(), body2.size());
		if (nparsed != body2.size())
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			return;
		}
	}
}

void test3()
{
	http_parser_settings settings;
	memset(&settings, 0, sizeof(settings));
	settings.on_message_begin = onMessageBegin;
	settings.on_url = onUrl;
	settings.on_header_field = onHeaderField;
	settings.on_header_value = onHeaderValue;
	settings.on_headers_complete = onHeadersComplete;
	settings.on_body = onBody;
	settings.on_message_complete = onMessageComplete;

	http_parser parser;
	http_parser_init(&parser, HTTP_REQUEST);
	Context ctx;
	ctx.name = "test3";
	parser.data = &ctx;
	for (size_t i = 0; i < 10; i++)
	{
		std::string url1 = "GET /correct";
		std::string url2 = "test.aspx?a=1&b=2 HTTP/1.0\r\n";
		std::vector<std::string> headers;
		headers.push_back("Ho");
		headers.push_back("st: localhost: 80\r\n");
		headers.push_back("Connec");
		headers.push_back("tion: Keep - Alive\r\n");
		headers.push_back("Cont");
		headers.push_back("ent-Type: text/plain\r\n\r\n");

		ctx.buffer.append(url1);
		size_t nparsed = http_parser_execute(&parser, &settings, ctx.buffer.data(), ctx.buffer.size());
		if (nparsed != ctx.buffer.size())
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			ctx.buffer.clear();
			return;
		}

		ctx.buffer.append(url2);
		nparsed = http_parser_execute(&parser, &settings, ctx.buffer.data(), ctx.buffer.size());
		if (nparsed != ctx.buffer.size())
		{
			fprintf(stderr,
				"Error: %s (%s)\n",
				http_errno_description(HTTP_PARSER_ERRNO(&parser)),
				http_errno_name(HTTP_PARSER_ERRNO(&parser)));
			ctx.buffer.clear();
			return;
		}

		for (size_t i = 0; i < headers.size(); i++)
		{
			std::string header = headers[i];
			ctx.buffer.append(header);
			nparsed = http_parser_execute(&parser, &settings, ctx.buffer.data(), ctx.buffer.size());
			if (nparsed != ctx.buffer.size())
			{
				fprintf(stderr,
					"Error: %s (%s)\n",
					http_errno_description(HTTP_PARSER_ERRNO(&parser)),
					http_errno_name(HTTP_PARSER_ERRNO(&parser)));
				ctx.buffer.clear();
				return;
			}
		}
	}
}

const char* url =
"GET /find?a=1&b=2 HTTP/1.0\r\n"
"Host: 127.1.0.1:80\r\n"
"Connection: Keep-Alive\r\n"
//"Content-Length: 10\r\n"
"Content-Type: text/plain\r\n\r\n";
//"body1body2";

size_t kOnce = 3;
net::TcpConnectionPtr g_conn;
base::CountDownLatch g_latch(1);

class Client : boost::noncopyable
{
public:
	Client(net::EventLoop* loop,
		   const net::InetAddress& listenAddr)
		: client_(loop, listenAddr, "Client")
	{
		client_.setConnectionCallback(
			boost::bind(&Client::onConnection, this, _1));
		client_.setMessageCallback(
			boost::bind(&Client::onMessage, this, _1, _2));
		client_.setWriteCompleteCallback(
			boost::bind(&Client::onWriteComplete, this, _1));
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
		LOG_INFO << conn->name() << " is " << (conn->connected() ? "UP" : "DOWN");
		if (conn->connected())
		{
			g_conn = conn;
			g_latch.countDown();
		}
		else
		{
			g_conn.reset();
		}
	}

	void onMessage(const net::TcpConnectionPtr& conn, net::Buffer* buffer)
	{
		std::string msg;
		buffer->retrieveAllAsString(&msg);
		client_.disconnect();
		LOG_INFO << msg;
	}

	void onWriteComplete(const net::TcpConnectionPtr& conn)
	{
		//printf("onWriteComplete\n"); 
		/*if (remain_ > 0)
		{
			usleep(1000);
			size_t len = remain_ < kOnce ? remain_ : kOnce;
			std::string msg(url + offset_, len);
			conn->send(msg);
			printf("%s", msg.c_str());
			offset_ += len;
			remain_ -= len;
		}*/
	}

	net::TcpClient client_;
	size_t offset_;
	size_t remain_;
};

void connect(Client* cli)
{
	cli->connect();
}

int main(int argc, char* argv[]) 
{
	base::Logger::setLogLevel(base::Logger::DEBUG);
	//test1();
	//test2();
	//test3();
	LOG_INFO << url;
	if (argc > 2)
	{
		net::EventLoop loop;

		net::EventLoopThread otherThread;
		net::InetAddress serverAddr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
		net::EventLoop* cliLoop = otherThread.startLoop();
		Client cli(cliLoop, serverAddr);
		cliLoop->runInLoop(boost::bind(connect, &cli));

		g_latch.wait();

		size_t offset_ = 0;
		size_t remain_ = strlen(url);

		do
		{
			size_t len = remain_ < kOnce ? remain_ : kOnce;
			std::string msg(url + offset_, len);
			g_conn->send(msg);
			offset_ += len;
			remain_ -= len;
			usleep(10000);
		} while (remain_ > 0);

		loop.loop();
	}
	else
	{
		LOG_ERROR << "Usage: " << argv[0] << " ip port\n";
		abort();
	}

	return 0;
}
