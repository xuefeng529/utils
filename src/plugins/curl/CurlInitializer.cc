#include "base/Logging.h"

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
            LOG_ERROR << "url_global_init()";
            sleep(3);
            abort();
        }
    }

    ~CurlInitializer()
    {
        curl_global_cleanup();
    }
};

CurlInitializer theCurlInitializer;

} // namespace curl
} // namespace plugins
