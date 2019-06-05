#ifndef PLUGINS_CURL_MULTIHTTPCLIENT_H
#define PLUGINS_CURL_MULTIHTTPCLIENT_H

#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>
#include <string>
#include <map>
#include <set>
#include <vector>

typedef void CURL;
typedef void CURLM;
struct curl_slist;

namespace plugins
{
namespace curl
{

class MultiHttpClient : boost::noncopyable
{
public:
	struct Request 
	{
		std::string url;		
		std::map<std::string, std::string> headers;
		std::string body;
		int timeout;
		boost::any ctx;
	};

	struct Result
	{
		std::string strerror;
		int statusCode;
		std::string response;
		Result() : strerror("unknown"), statusCode(-1) {}
	};

	MultiHttpClient(bool keepalive);
	~MultiHttpClient();

	void enableDebug() { enabledDebug_ = true; }
	void enableHttp2() { enabledHttp2_ = true; }
	void setCaFile(const std::string& caFile) { caFile_ = caFile; }
	void setCertFile(const std::string& certFile, const std::string& keyFile, const std::string& password)
	{
		certFile_ = certFile;
		keyFile_ = keyFile;
		keyPassword_ = password;
	}

	const std::string& getCertFile() const { return certFile_; }
	const std::string& getKeyFile() const { return keyFile_; }

    bool post(const std::vector<Request>& requests, std::vector<Result>* results);
	const std::string& strerror() const { return strerror_; }

private:
	struct CurlContext
	{
		CURL* curl;
		curl_slist* headers;
		int requestIndex;

		CurlContext();
		~CurlContext();
	};

	typedef boost::shared_ptr<CurlContext> CurlContextPtr;

    static size_t handleResponse(void* buf, size_t size, size_t nmemb, void* ctx);
    
	void resetConstOpt(CurlContext* ctx);
	void resetCommonOpt(CurlContext* ctx,
						const std::string& url,
						const std::map<std::string, std::string>& headers,
						int timeout,
						std::string* response);

	void resetPostOpt(CurlContext* ctx, const std::string& body);
	bool exec(std::vector<Result>* results);
	void clear();
	
	bool keepalive_;
	CURLM* curlm_;
	std::vector<CurlContextPtr> freeCurlContexts_;
	std::map<CurlContext*, CurlContextPtr> usedCurlContexts_;

	bool enabledDebug_;
	bool enabledHttp2_;
	
	std::string caFile_;
	std::string certFile_;
	std::string keyFile_;
	std::string keyPassword_;

	std::string strerror_;
};

} // namespace curl
} // namespace plugins

#endif // PLUGINS_CURL_MULTIHTTPCLIENT_H
