#include "net/http/HttpResponse.h"
#include "net/Buffer.h"

#include <stdio.h>

namespace net
{

HttpResponse::HttpResponse()
	: statusCode_(kUnknown)
{
}

void HttpResponse::setCloseConnection(bool on)
{
	const char* value = (on ? "close" : "Keep-Alive");
	addHeader("Connection", value);
}

bool HttpResponse::closeConnection() const
{
	std::map<std::string, std::string>::const_iterator it =
		headers_.find("Connection");
	return (it == headers_.end() || it->second == "close");
}

std::string HttpResponse::getHeader(const std::string& field) const
{
	std::string result;
	std::map<std::string, std::string>::const_iterator it = headers_.find(field);
	if (it != headers_.end())
	{
		result = it->second;
	}
	return result;
}

void HttpResponse::swap(HttpResponse* that)
{
	std::swap(statusCode_, that->statusCode_);
	statusMessage_.swap(that->statusMessage_);
	headers_.swap(that->headers_);
	body_.swap(that->body_);
}

bool HttpResponse::appendToBuffer(Buffer* output) const
{
	if (statusCode_ == kUnknown || statusMessage_.empty())
	{
		return false;
	}

	char buf[64];
	snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", statusCode_);
	output->append(buf);
	output->append(statusMessage_);
	output->append("\r\n");

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

} // namespace net
