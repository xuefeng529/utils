#ifndef NET_HTTP_HTTPRESPONSE_H
#define NET_HTTP_HTTPRESPONSE_H

#include <string>
#include <map>
#include <boost/function.hpp>

namespace net
{

class Buffer;

class HttpResponse
{
public:
	enum StatusCode
	{
		kUnknown,
		k200Ok = 200,
		k301MovedPermanently = 301,
		k302MovedTemporarily = 302,
		k400BadRequest = 400,
		k404NotFound = 404,
	};

	/// 第一个参数返回包体分块，第二个参数返回包体分块是否结束。
	typedef boost::function<bool(net::Buffer*, bool*)> ChunkedCallback;

	HttpResponse();

	void setStatusCode(StatusCode code)
	{ statusCode_ = code; }

	StatusCode statusCode() const
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

	void setBody(const char* buf, size_t len)
	{ body_.assign(buf, len); }

	void appendBody(const std::string& body)
	{ body_.append(body); }

	void appendBody(const char* buf, size_t len)
	{ body_.append(buf, len); }

	const std::string& body() const
	{ return body_; }

	void swap(HttpResponse* that);

	bool appendToBuffer(Buffer* output) const;

	void enableChunked(const ChunkedCallback& cb)
	{ chunkedCb_ = cb; }

	ChunkedCallback getChunkedCallback() const { return chunkedCb_; }

private:
	StatusCode statusCode_;
	std::string statusMessage_;
	std::map<std::string, std::string> headers_;
	std::string body_;
	ChunkedCallback chunkedCb_;
};

} // namespace net

#endif  // NET_HTTP_HTTPRESPONSE_H
