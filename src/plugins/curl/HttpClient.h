#ifndef PLUGINS_CURL_HTTPCLIENT_H
#define PLUGINS_CURL_HTTPCLIENT_H

#include <boost/noncopyable.hpp>
#include <string>

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
    /// @timeout 0 invalid
    bool get(const std::string& url, int timeout, std::string* response);
    bool post(const std::string& url, const std::string& data, int timeout, std::string* response = NULL);
    bool put(const std::string& url, const std::string& data, int timeout, std::string* response = NULL);
    bool del(const std::string& url, int timeout, std::string* response = NULL);
    
private:
    static size_t onResponse(void* contents, size_t size, size_t nmemb, void* userp);

    struct curl_slist* setConnection(bool keepalive);

    CURL *curl_;
    bool keepalive_;
};

} // namespace curl
} // namespace plugins

#endif // PLUGINS_CURL_HTTPCLIENT_H
