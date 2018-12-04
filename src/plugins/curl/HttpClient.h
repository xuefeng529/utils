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

    void setHeader(const std::string& name, const std::string& value);
    /// @timeout 0 invalid
    bool get(const std::string& url, int timeout, std::string* response);
    bool post(const std::string& url, const std::string& data, int timeout, std::string* response);
    bool put(const std::string& url, const std::string& data, int timeout, std::string* response);
    bool del(const std::string& url, int timeout, std::string* response);
    int statusCode() const { return statusCode_; }
    const std::string& statusMessage() const { return statusMessage_; }

private:
    static size_t handleResponse(void* buf, size_t size, size_t nmemb, void* ctx);
    static size_t handleHeader(void* buf, size_t size, size_t nmemb, void* ctx);

    struct curl_slist* setHeaders();

    CURL *curl_;
    std::map<std::string, std::string> headers_;
    bool keepalive_;
    int statusCode_;
    std::string statusMessage_;
};

} // namespace curl
} // namespace plugins

#endif // PLUGINS_CURL_HTTPCLIENT_H
