#include "plugins/curl/MultiHttpClient.h"

#include <algorithm>

#include <curl/curl.h>
#include <curl/multi.h>
#include <assert.h>

namespace plugins
{
namespace curl
{
	
/// Wait max 30 seconds
const int kMaxWaitMsecs = 30 * 1000;

/// seconds 
const int kTcpKeepIdle = 120; 
const int kTcpKeepIntv = 60;

namespace
{

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

	default: /// in case a new one is introduced to shock us
		return 0;
	}

	dump(text, stderr, (unsigned char *)data, size);
	return 0;
}

}

size_t MultiHttpClient::handleResponse(void* buf, size_t size, size_t nmemb, void* ctx)
{
    size_t realSize = size * nmemb;
    if (ctx != NULL)
    {
        std::string* response = static_cast<std::string*>(ctx);
        response->append(static_cast<const char*>(buf), realSize);
    }
    
    return realSize;
}

MultiHttpClient::MultiHttpClient(bool keepalive)
	: keepalive_(keepalive),
	  curlm_(curl_multi_init()),
	  enabledDebug_(false),
	  enabledHttp2_(false),
	  strerror_("unknown")
{  
}

MultiHttpClient::~MultiHttpClient()
{
	if (curlm_ != NULL)
    {
		curl_multi_cleanup(curlm_);
    }
}

MultiHttpClient::CurlContext::CurlContext()
	: curl(curl_easy_init()),
	headers(NULL),
	requestIndex(-1)
{
}

MultiHttpClient::CurlContext::~CurlContext()
{
	if (curl != NULL)
	{
		curl_easy_cleanup(curl);
		curl = NULL;
	}

	if (headers != NULL)
	{
		curl_slist_free_all(headers);
		headers = NULL;
	}
}

bool MultiHttpClient::post(const std::vector<Request>& requests, std::vector<Result>* results)
{
	assert(!requests.empty());
	assert(results != NULL);
	results->clear();
	results->resize(requests.size());
	for (size_t i = 0; i < requests.size(); i++)
	{		
		const Request& req = requests[i];
		CurlContextPtr ctx;
		std::vector<CurlContextPtr>::iterator it = freeCurlContexts_.begin();
		if (it == freeCurlContexts_.end())
		{
			ctx.reset(new CurlContext());
			resetConstOpt(ctx.get());
		}
		else
		{
			ctx = *it;
			freeCurlContexts_.erase(it);
		}

		usedCurlContexts_[ctx.get()] = ctx;
		ctx->requestIndex = i;
		resetCommonOpt(ctx.get(), req.url, req.headers, req.timeout, &(*results)[i].response);
		resetPostOpt(ctx.get(), req.body);
		curl_easy_setopt(ctx->curl, CURLOPT_PRIVATE, ctx.get());
		/// add the individual transfers
		curl_multi_add_handle(curlm_, ctx->curl);
	}

	return exec(results);
}

void MultiHttpClient::resetConstOpt(CurlContext* ctx)
{
	assert(ctx != NULL);
	assert(ctx->curl != NULL);
	if (enabledDebug_)
	{
		curl_easy_setopt(ctx->curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(ctx->curl, CURLOPT_DEBUGFUNCTION, handleDebug);
	}

	if (enabledHttp2_)
	{
		curl_easy_setopt(ctx->curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);
	}

	if (!caFile_.empty())
	{
		/// 验证服务器证书有效性
		curl_easy_setopt(ctx->curl, CURLOPT_SSL_VERIFYPEER, 1L);
		curl_easy_setopt(ctx->curl, CURLOPT_CAINFO, caFile_.c_str());
	}

	if (!certFile_.empty())
	{
		/// 客户端证书，用于双向认证
		curl_easy_setopt(ctx->curl, CURLOPT_SSLCERT, certFile_.c_str());
	}

	if (!keyFile_.empty())
	{
		/// 客户端证书私钥，用于双向认证
		curl_easy_setopt(ctx->curl, CURLOPT_SSLKEY, keyFile_.c_str());
	}

	if (!keyPassword_.empty())
	{
		/// 客户端证书私钥密码
		curl_easy_setopt(ctx->curl, CURLOPT_SSLCERTPASSWD, keyPassword_.c_str());
	}	
}

void MultiHttpClient::resetCommonOpt(CurlContext* ctx,
									 const std::string& url,
									 const std::map<std::string, std::string>& headers,
									 int timeout,
									 std::string* response)
								
{
	assert(ctx != NULL);
	assert(ctx->curl != NULL);
	assert(ctx->headers == NULL);
	curl_easy_setopt(ctx->curl, CURLOPT_URL, url.c_str());
	if (timeout != 0)
	{
		curl_easy_setopt(ctx->curl, CURLOPT_TIMEOUT, timeout);
		curl_easy_setopt(ctx->curl, CURLOPT_NOSIGNAL, 1L);
	}

	if (response != NULL)
	{
		curl_easy_setopt(ctx->curl, CURLOPT_WRITEFUNCTION, handleResponse);
		curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, response);
	}

	if (keepalive_)
	{
		/// enable TCP keep-alive for this transfer
		curl_easy_setopt(ctx->curl, CURLOPT_TCP_KEEPALIVE, 1L);
		/// keep-alive idle time to kTcpKeepIdle seconds
		curl_easy_setopt(ctx->curl, CURLOPT_TCP_KEEPIDLE, kTcpKeepIdle);
		/// interval time between keep-alive probes: kTcpKeepIntv seconds
		curl_easy_setopt(ctx->curl, CURLOPT_TCP_KEEPINTVL, kTcpKeepIntv);
		ctx->headers = curl_slist_append(ctx->headers, "Connection: keep-alive");
	}
	else
	{
		ctx->headers = curl_slist_append(ctx->headers, "Connection: close");
	}

	std::map<std::string, std::string>::const_iterator it = headers.begin();
	for (; it != headers.end(); ++it)
	{
		std::string header = it->first + ": " + it->second;
		ctx->headers = curl_slist_append(ctx->headers, header.c_str());
	}

	curl_easy_setopt(ctx->curl, CURLOPT_HTTPHEADER, ctx->headers);
}

void MultiHttpClient::resetPostOpt(CurlContext* ctx, const std::string& body)
{
	assert(ctx != NULL);
	assert(ctx->curl != NULL);
	curl_easy_setopt(ctx->curl, CURLOPT_POST, 1L);
	curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, body.data());
	curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDSIZE, body.size());
}

bool MultiHttpClient::exec(std::vector<Result>* results)
{
	assert(results != NULL);
	int stillRunning = 0;
	/// we start some action by calling perform right away
	CURLMcode res = curl_multi_perform(curlm_, &stillRunning);
	if (res != CURLM_OK)
	{
		fprintf(stderr, "curl_multi_perform() failed, error: %d\n", res);
		strerror_ = "curl_multi_perform() failed";
		clear();
		return false;
	}

	do
	{
		int numfds = 0;
		/// wait for activity, timeout or "nothing"
		res = curl_multi_wait(curlm_, NULL, 0, kMaxWaitMsecs, &numfds);
		if (res != CURLM_OK)
		{
			fprintf(stderr, "curl_multi_wait(), error: %d\n", res);
			strerror_ = "curl_multi_wait() failed";
			clear();
			return false;
		}

		res = curl_multi_perform(curlm_, &stillRunning);
		if (res != CURLM_OK)
		{
			fprintf(stderr, "curl_multi_perform() failed, error: %d\n", res);
			strerror_ = "curl_multi_perform() failed";
			clear();
			return false;
		}
	} while (stillRunning);

	bool ret = true;
	CURLMsg *msg = NULL;
	int numMsgs = 0;
	while ((msg = curl_multi_info_read(curlm_, &numMsgs)))
	{
		if (msg->msg == CURLMSG_DONE)
		{
			CURL* curl = msg->easy_handle;
			CurlContext* ctx = NULL;
			curl_easy_getinfo(curl, CURLINFO_PRIVATE, &ctx);
			CURLcode returnCode = msg->data.result;
			if (returnCode != CURLE_OK)
			{
				ret = false;
				(*results)[ctx->requestIndex].strerror = curl_easy_strerror(msg->data.result);
				curl_multi_remove_handle(curlm_, curl);
				usedCurlContexts_.erase(ctx);
				continue;
			}

			/// get http status code
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(*results)[ctx->requestIndex].statusCode);
			curl_multi_remove_handle(curlm_, curl);
			assert(ctx->headers != NULL);
			curl_slist_free_all(ctx->headers);
			ctx->headers = NULL;
			if (!keepalive_)
			{
				usedCurlContexts_.erase(ctx);
			}
			else
			{
				freeCurlContexts_.push_back(usedCurlContexts_[ctx]);
				usedCurlContexts_.erase(ctx);
			}
		}
		else
		{
			fprintf(stderr, "curl_multi_info_read failed, CURLMsg: %d\n", msg->msg);
		}
	}

	return ret;
}

void MultiHttpClient::clear()
{
	std::map<CurlContext*, CurlContextPtr>::iterator it = usedCurlContexts_.begin();
	for (; it != usedCurlContexts_.end(); ++it)
	{
		curl_multi_remove_handle(curlm_, it->second->curl);
	}
	
	usedCurlContexts_.clear();
}

} // namespace curl
} // namespace plugins
