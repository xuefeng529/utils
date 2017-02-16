#include "net/http/HttpContext.h"
#include "net/Buffer.h"
#include "base/StringUtil.h"
#include "base/Logging.h"

#include <vector>

namespace net
{

HttpContext::HttpContext(TcpConnectionWeakPtr weakConn, ParserType parserType)
	: weakConn_(weakConn),
	  offset_(0),
	  parserType_(parserType)
{
	http_parser_init(&parser_, static_cast<http_parser_type>(parserType_));
	parser_.data = this;
	http_parser_settings_init(&settings_);
	settings_.on_message_begin = handleMessageBegin;
	settings_.on_url = handleUrl;
	settings_.on_status = handleStatus;
	settings_.on_header_field = handleHeaderField;
	settings_.on_header_value = handleHeaderValue;
	settings_.on_headers_complete = handleHeadersComplete;
	settings_.on_body = handleBody;
	settings_.on_message_complete = handleMessageComplete;
}

HttpContext::HttpContext(const HttpContext& other)
	: weakConn_(other.weakConn_),
	  buffer_(other.buffer_),
	  offset_(other.offset_),
	  parserType_(other.parserType_),
	  parser_(other.parser_),
	  settings_(other.settings_),
	  request_(other.request_),
	  response_(other.response_),
	  lastHeaderField_(other.lastHeaderField_),
	  requestCallback_(other.requestCallback_),
	  responseCallback_(other.responseCallback_)
{
	parser_.data = this;
}

HttpContext& HttpContext::operator=(const HttpContext& other)
{
	if (&other == this)
	{
		return *this;
	}

	weakConn_ = other.weakConn_;
	buffer_ = other.buffer_;
	offset_ = other.offset_;
	parserType_ = other.parserType_;
	parser_ = other.parser_;
	parser_.data = this;
	settings_ = other.settings_;
	request_ = other.request_;
	response_ = other.response_;
	lastHeaderField_ = other.lastHeaderField_;
	requestCallback_ = other.requestCallback_;
	responseCallback_ = other.responseCallback_;
	return *this;
}

bool HttpContext::parse(Buffer* buffer)
{
	std::string input;
	buffer->retrieveAllAsString(&input);
	size_t len = input.size();
	buffer_.append(input);
	const char* begin = buffer_.data() + offset_;
	offset_ += len;
	size_t numParsed = http_parser_execute(&parser_, &settings_, begin, len);
	if (numParsed != len)
	{
		LOG_ERROR << buffer_ << ":" << http_errno_name(static_cast<http_errno>(parser_.http_errno))
			<< " " << http_errno_description(static_cast<http_errno>(parser_.http_errno));
		buffer_.clear();
		offset_ = 0;
		return false;
	}

	return true;
}

int HttpContext::handleMessageBegin(http_parser* parser)
{
	LOG_DEBUG << "message begin";
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	if (httpContext->parserType_ == kRequest)
	{
		HttpRequest tmp;
		httpContext->request_.swap(&tmp);
	}
	else
	{
		HttpResponse tmp;
		httpContext->response_.swap(&tmp);
	}

	return 0;
}

int HttpContext::handleUrl(http_parser* parser, const char *at, size_t length)
{
	std::string url(at, length);
	LOG_DEBUG << "url: " << url;
	std::string originalUrl;
	base::StringUtil::unescape(url, &originalUrl);
	LOG_DEBUG << "original url: " << originalUrl;
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	assert(httpContext->parserType_ == kRequest);
	httpContext->request_.setMethod(static_cast<HttpRequest::Method>(parser->method));
	httpContext->request_.parseUrl(originalUrl);
	return 0;
}

int HttpContext::handleStatus(http_parser* parser, const char *at, size_t length)
{
	std::string status(at, length);
	LOG_DEBUG << "status: " << status;
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	assert(httpContext->parserType_ == kResponse);
	httpContext->response_.setStatusCode(static_cast<HttpResponse::StatusCode>(parser->status_code));
	httpContext->response_.setStatusMessage(status);
	return 0;
}

int HttpContext::handleHeaderField(http_parser* parser, const char *at, size_t length)
{
	std::string field(at, length);
	LOG_DEBUG << "header field: " << field;
	static_cast<HttpContext*>(parser->data)->lastHeaderField_ = field;
	return 0;
}

int HttpContext::handleHeaderValue(http_parser* parser, const char *at, size_t length)
{
	std::string value(at, length);
	LOG_DEBUG << "header value: " << value;
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	if (httpContext->parserType_ == kRequest)
	{
		httpContext->request_.addHeader(httpContext->lastHeaderField_, value);
	}
	else
	{
		httpContext->response_.addHeader(httpContext->lastHeaderField_, value);
	}

    return 0;
}

int HttpContext::handleHeadersComplete(http_parser* parser)
{
	LOG_DEBUG << "headers complete: " << parser->nread;
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	if (httpContext->parserType_ == kRequest)
	{
		if (parser->http_major == 1 && parser->http_minor == 0)
		{
			LOG_DEBUG << "kHttp10";
			httpContext->request_.setVersion(HttpRequest::kHttp10);
		}
		else if (parser->http_major == 1 && parser->http_minor == 1)
		{
			LOG_DEBUG << "kHttp11";
			httpContext->request_.setVersion(HttpRequest::kHttp11);
		}
		else
		{
			LOG_DEBUG << "kUnknown";
			httpContext->request_.setVersion(HttpRequest::kUnknown);
		}
	}

    return 0;
}

int HttpContext::handleBody(http_parser* parser, const char *at, size_t length)
{
	std::string body(at, length);
	LOG_DEBUG << "body: " << body;
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	if (httpContext->parserType_ == kRequest)
	{
		httpContext->request_.setBody(body);
	}
	else
	{
		httpContext->response_.setBody(body);
	}

    return 0;
}

int HttpContext::handleMessageComplete(http_parser* parser)
{
	LOG_DEBUG << "message complete";
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	httpContext->buffer_.clear();
	httpContext->offset_ = 0;
	if (httpContext->parserType_ == kRequest)
	{
		TcpConnectionPtr conn = httpContext->weakConn_.lock();
		if (conn)
		{
			httpContext->requestCallback_(conn, httpContext->request_);
		}
	}
	else
	{
		TcpConnectionPtr conn = httpContext->weakConn_.lock();
		if (conn)
		{
			httpContext->responseCallback_(conn, httpContext->response_);
		}
	}

    return 0;
}

} // namespace net
