#ifndef PLUGIN_HTTP_HTTPCONTEXT_H
#define PLUGIN_HTTP_HTTPCONTEXT_H

#include "base/Buffer.h"
#include "plugins/http_lite/HttpRequest.h"

#include <boost/function.hpp>

namespace plugin
{
namespace http
{

typedef boost::function<void (const HttpRequest&)> HttpRequestCallback;

class HttpContext
{
public:
	enum HttpRequestParseState
	{
		kExpectRequestLine,
		kExpectHeaders,
		kExpectBody
	};

	HttpContext()
		: state_(kExpectRequestLine)
	{
	}

	void setRequestCallback(const HttpRequestCallback& cb)
	{
		requestCallback_ = cb;
	}

	void parseRequest(const char* buf, size_t len);

	void reset()
	{
		state_ = kExpectRequestLine;
		HttpRequest dummy;
		request_.swap(dummy);
	}

private:
	bool processRequestLine(const char* begin, const char* end);

	base::Buffer buffer_;
	HttpRequestParseState state_;
	HttpRequest request_;

	HttpRequestCallback requestCallback_;
};

} // namespace http
} // namespace plugin

#endif  // PLUGIN_HTTP_HTTPCONTEXT_H
