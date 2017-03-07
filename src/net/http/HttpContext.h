#ifndef NET_HTTP_HTTPCONTEXT_H
#define NET_HTTP_HTTPCONTEXT_H

#include "net/http/http_parser.h"
#include "net/TcpConnection.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"

#include <boost/function.hpp>
#include <string>

namespace net
{

class Buffer;

class HttpContext
{
public:
	typedef boost::function<void(const TcpConnectionPtr&, const HttpRequest&)> RequestCallback;
	typedef boost::function<void(const TcpConnectionPtr&, const HttpResponse&)> ResponseCallback;

	enum ParserType { kRequest, kResponse };

	typedef boost::weak_ptr<TcpConnection> TcpConnectionWeakPtr;
	HttpContext(TcpConnectionWeakPtr weakConn, ParserType parserType);

	HttpContext(const HttpContext& other);
	HttpContext& operator=(const HttpContext& other);

	void setRequestCallback(const RequestCallback& cb)
	{ requestCallback_ = cb; }

	void setResponseCallback(const ResponseCallback& cb)
	{ responseCallback_ = cb; }

	bool parse(Buffer* buffer);
    
private:
	static int handleMessageBegin(http_parser* parser);
	static int handleUrl(http_parser* parser, const char *at, size_t length);
	static int handleStatus(http_parser* parser, const char *at, size_t length);
	static int handleHeaderField(http_parser* parser, const char *at, size_t length);
	static int handleHeaderValue(http_parser* parser, const char *at, size_t length);
	static int handleHeadersComplete(http_parser* parser);
	static int handleBody(http_parser* parser, const char *at, size_t length);
	static int handleMessageComplete(http_parser* parser);

	enum ParserStatus
	{
		kUnknow = -1,
		kMessageBegin,
		kUrl,
		kStatus,
		kHeaderField,
		kHeaderValue,
		kHeadersComplete,
		kBody,
		kMessageComplete
	};

	TcpConnectionWeakPtr weakConn_;
	ParserStatus parserStatus_;
	ParserType parserType_;
    http_parser parser_;
    http_parser_settings settings_;
	HttpRequest request_;
	HttpResponse response_;
	std::string lastHeaderField_;
	size_t bodyRemain_;
	RequestCallback requestCallback_;
	ResponseCallback responseCallback_;
};

} // namespace net

#endif // NET_HTTP_HTTPCONTEXT_H
