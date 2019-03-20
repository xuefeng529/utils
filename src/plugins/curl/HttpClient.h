#ifndef PLUGINS_CURL_HTTPCLIENT_H
#define PLUGINS_CURL_HTTPCLIENT_H

#include <boost/noncopyable.hpp>
#include <string>
#include <map>

typedef void CURL;
struct curl_slist;

namespace plugins
{
namespace curl
{

class HttpClient : boost::noncopyable
{
public:
    HttpClient(bool keepalive);
    ~HttpClient();

	void enableDebug() { enabledDebug_ = true; }
	void enableHttp2() { enabledHttp2_ = true; }
	void setCaFile(const std::string& caFile) { caFile_ = caFile; }
	void setCertFile(const std::string& certFile, const std::string& keyFile, const std::string& password)
	{
		certFile_ = certFile;
		keyFile_ = keyFile;
		keyPassword_ = password;
	}

    void setHeader(const std::string& name, const std::string& value);
    /// @timeout 0 invalid
	bool get(const std::string& url, 
			 int timeout, 
			 std::string* response, 
			 std::map<std::string, std::string>* responseHeaders = NULL);

    bool post(const std::string& url, 
			  const std::string& data, 
			  int timeout, 
		      std::string* response, 
		      std::map<std::string, std::string>* responseHeaders = NULL);

	bool put(const std::string& url,
			 const std::string& data,
			 int timeout,
			 std::string* response,
			 std::map<std::string, std::string>* responseHeaders = NULL);

	bool del(const std::string& url,
			 int timeout,
			 std::string* response,
			 std::map<std::string, std::string>* responseHeaders = NULL);
   
	const std::string& strerror() const { return strerror_; }
	int statusCode() const { return statusCode_; }
	const std::string& statusMessage() const { return statusMessage_; }

private:
	static size_t handleResponseHeaders(void* buf, size_t size, size_t nmemb, void* ctx);
    static size_t handleResponse(void* buf, size_t size, size_t nmemb, void* ctx);
    
	void setDebug();
	void setHttp2();
	void setSsl();
    struct curl_slist* setRequestHeaders();
	
    CURL *curl_;

	bool enabledDebug_;
	bool enabledHttp2_;
	
	std::string caFile_;
	std::string certFile_;
	std::string keyFile_;
	std::string keyPassword_;

    std::map<std::string, std::string> requestHeaders_;

    bool keepalive_;

	std::string strerror_;

    int statusCode_;
    std::string statusMessage_;
};

} // namespace curl
} // namespace plugins

#endif // PLUGINS_CURL_HTTPCLIENT_H
