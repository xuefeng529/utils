#include "plugins/curl/HttpClient.h"

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

inline void setResponseHeaders(const char* buf, size_t len, std::map<std::string, std::string>* kvs)
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
    }
}

void dump(const char* text, FILE* stream, unsigned char* ptr, size_t size)
{
	size_t i;
	size_t c;
	unsigned int width = 0x10;

	fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
		text, (long)size, (long)size);

	for (i = 0; i < size; i += width)
	{
		fprintf(stream, "%4.4lx: ", (long)i);

		/// show hex to the left
		for (c = 0; c < width; c++)
		{
			if (i + c < size)
			{
				fprintf(stream, "%02x ", ptr[i + c]);
			}
			else
			{
				fputs("   ", stream);
			}		
		}

		/// show data on the right
		for (c = 0; (c < width) && (i + c < size); c++)
		{
			char x = (ptr[i + c] >= 0x20 && ptr[i + c] < 0x80) ? ptr[i + c] : '.';
			fputc(x, stream);
		}

		fputc('\n', stream);
	}
}

int handleDebug(CURL* curl, curl_infotype type, char* data, size_t size, void* ctx)
{
	(void)curl;
	(void)ctx;

	const char *text;
	switch (type)
	{
	case CURLINFO_TEXT:
		fprintf(stderr, "== Info: %s", data);
	default: /// in case a new one is introduced to shock us
		return 0;

	case CURLINFO_HEADER_OUT:
		text = "=> Send header";
		break;
	case CURLINFO_DATA_OUT:
		text = "=> Send data";
		break;
	case CURLINFO_SSL_DATA_OUT:
		text = "=> Send SSL data";
		break;
	case CURLINFO_HEADER_IN:
		text = "<= Recv header";
		break;
	case CURLINFO_DATA_IN:
		text = "<= Recv data";
		break;
	case CURLINFO_SSL_DATA_IN:
		text = "<= Recv SSL data";
		break;
	}

	dump(text, stderr, (unsigned char *)data, size);
	return 0;
}

}

size_t HttpClient::handleResponseHeaders(void* buf, size_t size, size_t nmemb, void* ctx)
{
    size_t realSize = size * nmemb;
    if (ctx != NULL)
    {
        std::map<std::string, std::string>* headers = static_cast<std::map<std::string, std::string>*>(ctx);
        setResponseHeaders(static_cast<const char*>(buf), realSize, headers);
    }

    return realSize;
}

size_t HttpClient::handleResponse(void* buf, size_t size, size_t nmemb, void* ctx)
{
    size_t realSize = size * nmemb;
    if (ctx != NULL)
    {
        std::string* response = static_cast<std::string*>(ctx);
        response->append(static_cast<const char*>(buf), realSize);
    }
    
    return realSize;
}

HttpClient::HttpClient(bool keepalive)
    : curl_(keepalive ? curl_easy_init() : NULL),
	  enabledDebug_(false),
	  enabledHttp2_(false),
      keepalive_(keepalive),	
	  strerror_("unknown"),
      statusMessage_("unknown")
{  
}

HttpClient::~HttpClient()
{
    if (curl_ != NULL)
    {
        curl_easy_cleanup(curl_);
    }
}

void HttpClient::setHeader(const std::string& name, const std::string& value)
{
	requestHeaders_[name] = value;
}

bool HttpClient::get(const std::string& url,
					 int timeout,
					 std::string* response,
					 std::map<std::string, std::string>* responseHeaders)
{
    if (keepalive_)
    {
        if (curl_ == NULL)
        {
            strerror_ = "curl_easy_init failed";
            return false;
        }

        curl_easy_reset(curl_);
    }
    else
    {
        curl_ = curl_easy_init();
        if (curl_ == NULL)
        {           
			strerror_ = "curl_easy_init failed";
            return false;
        }
    }
       
    response->clear();
	setDebug();
	setHttp2();
	setSsl();
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
	struct curl_slist* requestHeaders = setRequestHeaders();
	if (responseHeaders != NULL)
	{
		curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, handleResponseHeaders);
		curl_easy_setopt(curl_, CURLOPT_HEADERDATA, responseHeaders);
	}

    if (response != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, handleResponse);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, response);
    }
    
    if (timeout != 0)
    {
        /// complete within timeout seconds
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    }

    CURLcode res = curl_easy_perform(curl_);
	if (res != CURLE_OK)
	{
		strerror_ = curl_easy_strerror(res);		
	}
	else
	{
		curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &statusCode_);
	}
	
	if (requestHeaders != NULL)
    {
		curl_slist_free_all(requestHeaders);
    }

    if (!keepalive_)
    {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }
   
    return (res == CURLE_OK);
}

bool HttpClient::post(const std::string& url,
					  const std::string& data,
					  int timeout,
					  std::string* response,
					  std::map<std::string, std::string>* responseHeaders)
{
    if (keepalive_)
    {
        if (curl_ == NULL)
        {
            strerror_ = "curl_easy_init failed";
            return false;
        }

        curl_easy_reset(curl_);
    }
    else
    {
        curl_ = curl_easy_init();
        if (curl_ == NULL)
        {
			strerror_ = "curl_easy_init failed";
            return false;
        }
    }
       
    response->clear();
	setDebug();
	setHttp2();
	setSsl();
	curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
	struct curl_slist* requestHeaders = setRequestHeaders();
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data()); 
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.size());
	if (responseHeaders != NULL)
	{
		curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, handleResponseHeaders);
		curl_easy_setopt(curl_, CURLOPT_HEADERDATA, responseHeaders);
	}

    if (response != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, handleResponse);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, response);
    }
    
    if (timeout != 0)
    {
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    }

    CURLcode res = curl_easy_perform(curl_);
	if (res != CURLE_OK)
	{
		strerror_ = curl_easy_strerror(res);	
	}
	else
	{
		curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &statusCode_);
	}

	if (requestHeaders != NULL)
    {
		curl_slist_free_all(requestHeaders);
    }

    if (!keepalive_)
    {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }

    return (res == CURLE_OK);
}

bool HttpClient::put(const std::string& url,
					 const std::string& data,
					 int timeout,
					 std::string* response,
					 std::map<std::string, std::string>* responseHeaders)
{
    if (keepalive_)
    {
        if (curl_ == NULL)
        {
            strerror_ = "curl_easy_init failed";
            return false;
        }

        curl_easy_reset(curl_);
    }
    else
    {
        curl_ = curl_easy_init();
        if (curl_ == NULL)
        {
			strerror_ = "curl_easy_init failed";
            return false;
        }
    }
   
    response->clear();
	setDebug();
	setHttp2();
	setSsl();
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str()); 
	struct curl_slist* requestHeaders = setRequestHeaders();
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.data());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, data.size());
	if (responseHeaders != NULL)
	{
		curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, handleResponseHeaders);
		curl_easy_setopt(curl_, CURLOPT_HEADERDATA, responseHeaders);
	}

    if (response != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, handleResponse);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, response);
    }

    if (timeout != 0)
    {
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    }

    CURLcode res = curl_easy_perform(curl_);
	if (res != CURLE_OK)
	{
		strerror_ = curl_easy_strerror(res);
	}
	else
	{
		curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &statusCode_);
	}
	
	if (requestHeaders != NULL)
    {
		curl_slist_free_all(requestHeaders);
    }

    if (!keepalive_)
    {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }

    return (res == CURLE_OK);
}

bool HttpClient::del(const std::string& url,
					 int timeout,
					 std::string* response,
					 std::map<std::string, std::string>* responseHeaders)
{
    if (keepalive_)
    {
        if (curl_ == NULL)
        {
            strerror_ = "curl_easy_init failed";
            return false;
        }

        curl_easy_reset(curl_);
    }
    else
    {
        curl_ = curl_easy_init();
        if (curl_ == NULL)
        {
			strerror_ = "curl_easy_init failed";
            return false;
        }
    }
   
    response->clear();
	setDebug();
	setHttp2();
	setSsl();
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
	struct curl_slist* requestHeaders = setRequestHeaders();
	if (responseHeaders != NULL)
	{
		curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, handleResponseHeaders);
		curl_easy_setopt(curl_, CURLOPT_HEADERDATA, responseHeaders);
	}

    if (response != NULL)
    {
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, handleResponse);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, response);
    }

    if (timeout != 0)
    {
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
    }

    CURLcode res = curl_easy_perform(curl_);
	if (res != CURLE_OK)
	{
		strerror_ = curl_easy_strerror(res);
	}
	else
	{
		curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &statusCode_);
	}

	if (requestHeaders != NULL)
    {
		curl_slist_free_all(requestHeaders);
    }

    if (!keepalive_)
    {
        curl_easy_cleanup(curl_);
        curl_ = NULL;
    }

    return (res == CURLE_OK);
}

void HttpClient::setDebug()
{
	if (enabledDebug_)
	{
		curl_easy_setopt(curl_, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl_, CURLOPT_DEBUGFUNCTION, handleDebug);
	}
}

void HttpClient::setHttp2()
{
	if (enabledHttp2_)
	{
		curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);
	}
}

void HttpClient::setSsl()
{
	if (!caFile_.empty())
	{
		/// 验证服务器证书有效性
		curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(curl_, CURLOPT_CAINFO, caFile_.c_str());
	}

	if (!certFile_.empty())
	{
		/// 客户端证书，用于双向认证
		curl_easy_setopt(curl_, CURLOPT_SSLCERT, certFile_.c_str());
	}

	if (!keyFile_.empty())
	{
		/// 客户端证书私钥，用于双向认证
		curl_easy_setopt(curl_, CURLOPT_SSLKEY, keyFile_.c_str());
	}

	if (!keyPassword_.empty())
	{
		/// 客户端证书私钥密码
		curl_easy_setopt(curl_, CURLOPT_SSLCERTPASSWD, keyPassword_.c_str());
	}
}

struct curl_slist* HttpClient::setRequestHeaders()
{
    struct curl_slist *headerList = NULL;
    if (keepalive_)
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

	std::map<std::string, std::string>::const_iterator it = requestHeaders_.begin();
	for (; it != requestHeaders_.end(); ++it)
    {
        std::string header = it->first + ": " + it->second;
		headerList = curl_slist_append(headerList, header.c_str());
    }

    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headerList);
    return headerList;
}

} // namespace curl
} // namespace plugins
