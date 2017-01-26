#ifndef NET_HTTP_HTTPCONTEXT_H
#define NET_HTTP_HTTPCONTEXT_H

#include "net/http2/http_parser.h"
#include "net/http2/HttpRequest.h"
#include "net/http2/HttpResponse.h"
#include "net/TcpConnection.h"

#include <boost/function.hpp>
#include <string>

namespace net
{

class Buffer;

class HttpContext
{
public:
	typedef boost::function<void(const HttpRequest&)> RequestCallback;
	typedef boost::function<void(const HttpResponse&)> ResponseCallback;

	enum ParserType { kRequest, kResponse };

	explicit HttpContext(ParserType parserType);

	void setRequestCallback(const RequestCallback& cb)
	{ requestCallback_ = cb; }

	void setResponseCallback(const ResponseCallback& cb)
	{ responseCallback_ = cb; }

	bool parse(Buffer* buf);
    
private:
	static int handleMessageBegin(http_parser* parser);
	static int handleUrl(http_parser* parser, const char *at, size_t length);
	static int handleStatus(http_parser* parser, const char *at, size_t length);
	static int handleHeaderField(http_parser* parser, const char *at, size_t length);
	static int handleHeaderValue(http_parser* parser, const char *at, size_t length);
	static int handleHeadersComplete(http_parser* parser);
	static int handleBody(http_parser* parser, const char *at, size_t length);
	static int handleMessageComplete(http_parser* parser);

	ParserType parserType_;
    http_parser parser_;
    http_parser_settings settings_;
	HttpRequest request_;
	HttpResponse response_;
	std::string currentHeaderField_;

	RequestCallback requestCallback_;
	ResponseCallback responseCallback_;
};

typedef boost::shared_ptr<HttpContext> HttpContextPtr;

} // namespace net

#endif // NET_HTTP_HTTPCONTEXT_H
