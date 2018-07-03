#ifndef PLUGINS_CURL_HTTPCLIENT_H
#define PLUGINS_CURL_HTTPCLIENT_H

#include <boost/noncopyable.hpp>
#include <string>

typedef void CURL;

namespace plugins
{
namespace curl
{

class HttpClient : boost::noncopyable
{
public:
    HttpClient(bool keepalive);
    ~HttpClient();

    bool get(const std::string& url, std::string* response);
    bool post(const std::string& url, const std::string& data, std::string* response = NULL);
    bool put(const std::string& url, const std::string& data, std::string* response = NULL);
    bool del(const std::string& url, std::string* response = NULL);
    
private:
    static size_t onResponse(void* contents, size_t size, size_t nmemb, void* userp);

    void enableKeepalive();

    CURL *curl_;
};

} // namespace curl
} // namespace plugins

#endif // PLUGINS_CURL_HTTPCLIENT_H
