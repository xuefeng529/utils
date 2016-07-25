#include "plugins/http_lite/HttpContext.h"
#include "plugins/http_lite/HttpResponse.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>

#include <iostream>

char g_buf[] = "it is very good !";

class TcpConnection
{
public:
	TcpConnection()
	{
		context_.setRequestCallback(boost::bind(&TcpConnection::handleRequest, this, _1));
	}

	void onMessage(char* buf, size_t len)
	{
		context_.parseRequest(buf, len);
	}

private:
	void handleRequest(const plugin::http::HttpRequest& request)
	{
		std::cout << "It is http request ..." << std::endl;
		std::cout << "Method: " << request.methodString() << std::endl;
		std::cout << "Url: " << request.path() << std::endl;
		std::cout << "query: " << request.query() << std::endl;
		std::cout << "Version: " << request.getVersion() << std::endl;
		std::map<std::string, std::string>::const_iterator it = request.headers().begin();
		for (; it != request.headers().end(); ++it)
		{
			std::cout << it->first << ": " << it->second << std::endl;
		}
		std::cout << "Body: " << request.body() << std::endl;

		std::cout << "It is http response ..." << std::endl;
		plugin::http::HttpResponse resp(true);
		resp.setStatusCode(plugin::http::HttpResponse::k200Ok);
		resp.setStatusMessage("it is ok");
		resp.setBody(g_buf, strlen(g_buf));
		base::Buffer buf;
		resp.appendToBuffer(&buf);
		std::cout << buf.retrieveAllAsString() << std::endl;
	}

	plugin::http::HttpContext context_;
};

int main()
{
	char httpRequest[] = "GET /index.html?user=xf&&index=1 HTTP/1.1\r\n"
		"Host: www.EastMoney.com\r\n"
		"User - Agent: Mozilla / 5.0 (Windows; U; Windows NT 5.1; zh - CN; rv:1.9.2.3) Gecko / 20100401 Firefox / 3.6.3\r\n"
		"Accept: text / html, application / xhtml + xml, application / xml; q = 0.9, */*;q=0.8\r\n"
		"Accept-Language: zh-cn,zh;q=0.5\r\n"
		"Accept-Encoding: gzip,deflate\r\n"
		"Accept-Charset: GB2312,utf-8;q=0.7,*;q=0.7\r\n"
		"Keep-Alive: 115\r\n"
		"Connection: keep-alive\r\n"
		"Content-Length: 13\r\n"
		"\r\n"
		"Hello world !";
		
	int cout = 1;
	TcpConnection conn;
	while (cout <= 100000)
	{
		std::cout << cout++ << std::endl;
		conn.onMessage(httpRequest, strlen(httpRequest));
	}
	
	while (true)
	{
		usleep(1000);
	}
	
	return 0;
}