#include "plugins/curl/HttpClient.h"
#include "base/Logging.h"

#include <algorithm>

#include <curl/curl.h>

namespace plugins
{
namespace curl
{

/// seconds 
const int kTcpKeepIdle = 120; 
const int kTcpKeepIntv = 60;

namespace
{

inline void addHeader(const char* buf, size_t len, std::map<std::string, std::string>* kvs)
{
    const char* end = buf + len;
    const char* colon = std::find(buf, end, ':');
    if (colon != end)
    {
        std::string field(buf, colon);
        ++colon;
        while (colon < end && isspace(*colon))
        {
            ++colon;
        }
        std::string value(colon, end);
        while (!value.empty() && isspace(value[value.size() - 1]))
        {
            value.resize(value.size() - 1);
        }

        (*kvs)[field] = value;
        LOG_INFO << field << ":" << value;
    }
}

}

size_t HttpClient::handleHeader(void* buf, size_t size, size_t nmemb, void* ctx)
{
    size_t realSize = size * nmemb;
    if (ctx != NULL)
    {
        std::map<std::string, std::string>* headers = static_cast<std::map<std::string, std::string>*>(ctx);
        addHeader(static_cast<const char*>(buf), realSize, headers);
    }

    return realSize;
}

size_t HttpClient::handleResponse(void* buf, size_t size, size_t nmemb, void* ctx)
{
    size_t realSize = size * nmemb;
    if (ctx != NULL)
    {
        std::string* response = static_cast<std::string*>(ctx);
        response->assign(static_cast<const char*>(buf), realSize);
    }
    
    return realSize;
}

HttpClient::HttpClient(bool keepalive)
    : curl_(keepalive ? curl_easy_init() : NULL),
      keepalive_(keepalive),
      code_(0)
{
    if (keepalive_ && curl_ == NULL)
    {
        LOG_ERROR << "curl_easy_init()";
        sleep(3);
        abort();
    }
}

HttpClient::~HttpClient()
{
    if (curl_ != NULL)
    {
        curl_easy_cleanup(curl_);
    }
}

bool HttpClient::get(const std::string& url, 
                     int timeout, 
                     std::string* response, 
                     std::map<std::string, std::string>* headers,
                     bool timeoutLog)
{
    if (keepalive_)
    {
        curl_easy_reset(curl_);
    }
    else
    {
        curl_ = curl_easy_init();
        if (curl_ == NULL)
        {
            LOG_ERROR << "curl_easy_init()";
            return false;
        }
    }
    
    struct curl_slist* headerList = setConnection(keepalive_); 
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    if (timeout != 0)
    {
        /// complete within timeout seconds
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    }

    if (headers != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, handleHeader);      
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, headers);
    }
    
    /// tell libcurl to follow redirection
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, handleResponse);
    if (response != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, response);
    }
    
    /// some servers don't like requests that are made without a user-agent
    /// field, so we provide one
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    CURLcode res = curl_easy_perform(curl_);
    if (headerList != NULL)
    {
        curl_slist_free_all(headerList);
    }

    if (!keepalive_)
    {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }
    
    if (res != CURLE_OK)
    {
        if (res != CURLE_OPERATION_TIMEDOUT || timeoutLog)
        {
            LOG_ERROR << "curl_easy_perform(): " << curl_easy_strerror(res);
        }
        
        code_ = static_cast<int>(res);
        return false;
    }

    return true;
}

bool HttpClient::post(const std::string& url, const std::string& data, int timeout, std::string* response)
{
    if (keepalive_)
    {
        curl_easy_reset(curl_);
    }
    else
    {
        curl_ = curl_easy_init();
        if (curl_ == NULL)
        {
            LOG_ERROR << "curl_easy_init()";
            return false;
        }
    }
    
    struct curl_slist* headerList = setConnection(keepalive_);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    if (timeout != 0)
    {
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
    }

    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data()); 
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, handleResponse);
    if (response != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, response);
    }
    
    CURLcode res = curl_easy_perform(curl_);
    if (headerList != NULL)
    {
        curl_slist_free_all(headerList);
    }

    if (!keepalive_)   
    {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }

    if (res != CURLE_OK)
    { 
        LOG_ERROR << "curl_easy_perform(): " << curl_easy_strerror(res);
        code_ = static_cast<int>(res);
        return false;
    }

    return true;
}

bool HttpClient::put(const std::string& url, const std::string& data, int timeout, std::string* response)
{
    if (keepalive_)
    {
        curl_easy_reset(curl_);
    }
    else
    {
        curl_ = curl_easy_init();
        if (curl_ == NULL)
        {
            LOG_ERROR << "curl_easy_init()";
            return false;
        }
    }

    struct curl_slist* headerList = setConnection(keepalive_);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
    if (timeout != 0)
    {
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
    }

    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.size());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, handleResponse);
    if (response != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, response);
    }

    CURLcode res = curl_easy_perform(curl_);
    if (headerList != NULL)
    {
        curl_slist_free_all(headerList);
    }

    if (!keepalive_)
    {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }

    if (res != CURLE_OK)
    {
        LOG_ERROR << "curl_easy_perform(): " << curl_easy_strerror(res);
        code_ = static_cast<int>(res);
        return false;
    }

    return true;
}

bool HttpClient::del(const std::string& url, int timeout, std::string* response)
{
    if (keepalive_)
    {
        curl_easy_reset(curl_);
    }
    else
    {
        curl_ = curl_easy_init();
        if (curl_ == NULL)
        {
            LOG_ERROR << "curl_easy_init()";
            return false;
        }
    }

    struct curl_slist* headerList = setConnection(keepalive_);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    if (timeout != 0)
    {
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
    }

    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, handleResponse);
    if (response != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, response);
    }

    CURLcode res = curl_easy_perform(curl_);
    if (headerList != NULL)
    {
        curl_slist_free_all(headerList);
    }

    if (!keepalive_)
    {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }
    
    if (res != CURLE_OK)
    {
        LOG_ERROR << "curl_easy_perform(): " << curl_easy_strerror(res);
        code_ = static_cast<int>(res);
        return false;
    }

    return true;
}

struct curl_slist* HttpClient::setConnection(bool keepalive)
{
    struct curl_slist *headerList = NULL;
    if (keepalive)
    {
        /// enable TCP keep-alive for this transfer
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
        /// keep-alive idle time to kTcpKeepIdle seconds
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, kTcpKeepIdle);
        /// interval time between keep-alive probes: kTcpKeepIntv seconds
        curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, kTcpKeepIntv);
        headerList = curl_slist_append(headerList, "Connection: keep-alive");
    }
    else
    {
        headerList = curl_slist_append(headerList, "Connection: close");
    }
    
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headerList);
    return headerList;
}

} // namespace curl
} // namespace plugins
