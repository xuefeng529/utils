#include <boost/noncopyable.hpp>

#include <curl/curl.h>

namespace plugins
{
namespace curl
{

class CurlInitializer : boost::noncopyable
{
public:
    CurlInitializer()
    {
        CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
        if (res != CURLE_OK)
        {
			fprintf(stderr, "url_global_init failed\n");           
            abort();
        }

		fprintf(stdout, "libcurl version: %s\n", curl_version());
    }

    ~CurlInitializer()
    {
        curl_global_cleanup();
    }
};

CurlInitializer theCurlInitializer;

} // namespace curl
} // namespace plugins
