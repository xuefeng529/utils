#ifndef NET_HTTP_HTTPRESPONSE_H
#define NET_HTTP_HTTPRESPONSE_H

#include "net/http2/HttpRequest.h"

#include <string>
#include <map>

namespace net
{

class Buffer;

class HttpResponse
{
public:
	enum HttpStatusCode
	{
		kUnknown,
		k200Ok = 200,
		k301MovedPermanently = 301,
		k400BadRequest = 400,
		k404NotFound = 404,
	};

	HttpResponse();

	void setVersion(HttpRequest::Version version)
	{ version_ = version; }

	HttpRequest::Version version() const
	{ return version_; }

	void setStatusCode(HttpStatusCode code)
	{ statusCode_ = code; }

	HttpStatusCode statusCode() const
	{ return statusCode_; }

	void setStatusMessage(const std::string& message)
	{ statusMessage_ = message; }

	const std::string& statusMessage() const
	{ return statusMessage_; }

	void setCloseConnection(bool on);

	bool closeConnection() const;

	void setContentType(const std::string& contentType)
	{ addHeader("Content-Type", contentType); }

	void addHeader(const std::string& key, const std::string& value)
	{ headers_[key] = value; }

	std::string getHeader(const std::string& field) const;

	void setBody(const std::string& body)
	{ body_ = body; }

	const std::string& body() const
	{ return body_; }

	void swap(HttpResponse* that);

	bool appendToBuffer(Buffer* output) const;

private:
	HttpRequest::Version version_;
	HttpStatusCode statusCode_;
	std::string statusMessage_;
	std::map<std::string, std::string> headers_;
	std::string body_;
};

} // namespace net

#endif  // NET_HTTP_HTTPRESPONSE_H
