#ifndef NET_HTTP_HTTPREQUEST_H
#define NET_HTTP_HTTPREQUEST_H

#include <string>
#include <map>

namespace net
{

class Buffer;

class HttpRequest
{
public:
	enum Method
	{
		kInvalid = -1, kDelete, kGet, kHead, kPost, kPut
	};

	enum Version
	{
		kUnknown = -1, kHttp10, kHttp11
	};

	HttpRequest();
	
	void setMethod(Method method)
	{ method_ = method; }

	Method method() const
	{ return method_; }

	const char* methodString() const;

	void parseUrl(const std::string& url);

	void setPath(const std::string& path)
	{ path_ = path; }

	const std::string& path() const
	{ return path_; }

	void setQuery(const std::string& query)
	{ query_ = query; }

	const std::string& query() const
	{ return query_; }

	void setVersion(Version version)
	{ version_ = version; }

	Version version() const
	{ return version_; }

	const char* versionString() const;

	void setCloseConnection(bool on);

	bool closeConnection() const;

	void addHeader(const std::string& field, const std::string& value)
	{ headers_[field] = value; }

	std::string getHeader(const std::string& field) const;

	const std::map<std::string, std::string>& headers() const
	{ return headers_; }

	void setBody(const std::string& body)
	{ body_ = body; }

	const std::string& body() const
	{ return body_; }

	void swap(HttpRequest* that);

	bool appendToBuffer(Buffer* output) const;

private:
	Method method_;
	std::string path_;
	std::string query_;
	Version version_;
	std::map<std::string, std::string> headers_;
	std::string body_;
};

} // namespace net

#endif // NET_HTTP_HTTPREQUEST_H
