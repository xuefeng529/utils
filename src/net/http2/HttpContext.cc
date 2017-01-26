#include "net/http2/HttpContext.h"
#include "net/Buffer.h"
#include "base/StringUtil.h"
#include "base/Logging.h"

#include <vector>

namespace net
{

HttpContext::HttpContext(ParserType parserType)
	: parserType_(parserType)
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

bool HttpContext::parse(Buffer* buf)
{
	std::string line;
	while (buf->readLine(&line))
	{
		line += "\r\n";
		size_t numParsed = http_parser_execute(&parser_, &settings_, line.data(), line.size());
		if (numParsed != line.size())
		{
			LOG_ERROR << "http_parser_execute(): " << line;
			return false;
		}
	}

	return true;
}

int HttpContext::handleMessageBegin(http_parser* parser)
{
	LOG_INFO << "message begin";
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
	LOG_INFO << "url: " << url;
	struct http_parser_url u;
	if (http_parser_parse_url(at, length, 0, &u) != 0)
	{
		LOG_ERROR << "parseurl error " << url;
		return 0;
	}

	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	assert(httpContext->parserType_ == kRequest);
	httpContext->request_.setMethod(static_cast<HttpRequest::Method>(parser->method));

	if (u.field_set & (1 << UF_PATH))
	{
		httpContext->request_.setPath(
			url.substr(u.field_data[UF_PATH].off, u.field_data[UF_PATH].len));
	}

	if (u.field_set & (1 << UF_QUERY))
	{
		httpContext->request_.setQuery(
			url.substr(u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len));
	}

	return 0;
}

int HttpContext::handleStatus(http_parser* parser, const char *at, size_t length)
{
	std::string status(at, length);
	LOG_INFO << "status: " << status;
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	assert(httpContext->parserType_ == kResponse);
	if (parser->http_major == 1 && parser->http_minor == 0)
	{
		httpContext->response_.setVersion(HttpRequest::kHttp10);
	}
	else if (parser->http_major == 1 && parser->http_minor == 1)
	{
		httpContext->response_.setVersion(HttpRequest::kHttp11);
	}
	else
	{
		httpContext->response_.setVersion(HttpRequest::kUnknown);
	}
	httpContext->response_.setStatusCode(static_cast<HttpResponse::HttpStatusCode>(parser->status_code));
	httpContext->response_.setStatusMessage(std::string(at, length));
	return 0;
}

int HttpContext::handleHeaderField(http_parser* parser, const char *at, size_t length)
{
	std::string headerValue(at, length);
	LOG_INFO << "header field: " << headerValue;
	static_cast<HttpContext*>(parser->data)->currentHeaderField_ = std::string(at, length);
	return 0;
}

int HttpContext::handleHeaderValue(http_parser* parser, const char *at, size_t length)
{
	std::string headerValue(at, length);
	LOG_INFO << "header value: " << headerValue;
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	if (httpContext->parserType_ == kRequest)
	{
		httpContext->request_.addHeader(httpContext->currentHeaderField_, std::string(at, length));
	}
	else
	{
		httpContext->response_.addHeader(httpContext->currentHeaderField_, std::string(at, length));
	}
    return 0;
}

int HttpContext::handleHeadersComplete(http_parser* parser)
{
	LOG_INFO << "headers complete: " << parser->nread << ":" << parser->content_length;
    return 0;
}

int HttpContext::handleBody(http_parser* parser, const char *at, size_t length)
{
	std::string body(at, length);
	LOG_INFO << "body: " << body;
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	if (httpContext->parserType_ == kRequest)
	{
		httpContext->request_.setBody(std::string(at, length));
	}
	else
	{
		httpContext->response_.setBody(std::string(at, length));
	}
    return 0;
}

int HttpContext::handleMessageComplete(http_parser* parser)
{
	LOG_INFO << "message complete";
	HttpContext* httpContext = static_cast<HttpContext*>(parser->data);
	if (httpContext->parserType_ == kRequest)
	{
		if (parser->http_major == 1 && parser->http_minor == 0)
		{
			httpContext->request_.setVersion(HttpRequest::kHttp10);
		}
		else if (parser->http_major == 1 && parser->http_minor == 1)
		{
			httpContext->request_.setVersion(HttpRequest::kHttp11);
		}
		else
		{
			httpContext->request_.setVersion(HttpRequest::kUnknown);
		}

		httpContext->requestCallback_(httpContext->request_);
	}
	else
	{
		httpContext->responseCallback_(httpContext->response_);
	}
    return 0;
}

} // namespace net
