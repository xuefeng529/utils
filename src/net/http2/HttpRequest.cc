#include "net/http2/http_parser.h"
#include "net/http2/HttpRequest.h"
#include "net/Buffer.h"
#include "base/Logging.h"
#include "base/StringUtil.h"

#include <vector>

namespace net
{

HttpRequest::HttpRequest()
	: method_(kInvalid),
	  version_(kHttp11)
{
}

HttpRequest::HttpRequest(Method method, const std::string& url, Version version)
	: method_(method),
	  version_(version)
{
	parseUrl(url);
}

HttpRequest::HttpRequest(Method method, const std::string& path, const std::string& query, Version version)
	: method_(method),
	  path_(path),
	  query_(query),
	  version_(version)
{
}

const char* HttpRequest::methodString() const
{
	const char* result = "UNKNOWN";
	switch (method_)
	{
	case kGet:
		result = "GET";
		break;
	case kPost:
		result = "POST";
		break;
	case kHead:
		result = "HEAD";
		break;
	case kPut:
		result = "PUT";
		break;
	case kDelete:
		result = "DELETE";
		break;
	default:
		break;
	}
	return result;
}

const char* HttpRequest::versionString() const
{
	const char* result = "UNKNOWN";
	switch (version_)
	{
	case kHttp10:
		result = "HTTP/1.0";
		break;
	case kHttp11:
		result = "HTTP/1.1";
		break;
	default:
		break;
	}
	return result;
}

std::string HttpRequest::getHeader(const std::string& field) const
{
	std::string result;
	std::map<std::string, std::string>::const_iterator it = headers_.find(field);
	if (it != headers_.end())
	{
		result = it->second;
	}
	return result;
}

void HttpRequest::swap(HttpRequest* that)
{
	std::swap(method_, that->method_);
	path_.swap(that->path_);
	query_.swap(that->query_);
	std::swap(version_, that->version_);
	headers_.swap(that->headers_);
	body_.swap(that->body_);
}

bool HttpRequest::appendToBuffer(Buffer* output) const
{
	if (method_ == kInvalid || path_.empty())
	{
		return false;
	}

	output->append(methodString());
	output->append(std::string(" ") + path_ + "?" + query_);
	if (version_ == kHttp10)
	{
		output->append(" HTTP/1.0\r\n");
	}
	else
	{
		output->append(" HTTP/1.1\r\n");
	}

	for (std::map<std::string, std::string>::const_iterator it = headers_.begin();
		it != headers_.end();
		++it)
	{
		output->append(it->first);
		output->append(": ");
		output->append(it->second);
		output->append("\r\n");
	}

	if (!body_.empty())
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "Content-Length: %zd\r\n", body_.size());
		output->append(buf);
		output->append("\r\n");
		output->append(body_);
	}
	else
	{
		output->append("\r\n");
	}

	return true;
}

void HttpRequest::parseUrl(const std::string& url)
{
	struct http_parser_url u;
	if (http_parser_parse_url(url.c_str(), url.size(), 0, &u) != 0)
	{
		LOG_ERROR << "parseurl error " << url;
		return;
	}

	std::string host;
	if (u.field_set & (1 << UF_HOST))
	{
		host = url.substr(u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
	}

	uint16_t port = 80;
	if (u.field_set & (1 << UF_PORT))
	{
		port = u.port;
	}

	headers_["Host"] = host + ":" + base::StringUtil::uint32ToStr(port);

	if (u.field_set & (1 << UF_PATH))
	{
		path_ = url.substr(u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);
	}

	if (u.field_set & (1 << UF_QUERY))
	{
		query_ = url.substr(u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
	}
}

} // namespace net
