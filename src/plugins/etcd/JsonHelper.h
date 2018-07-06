#ifndef PLUGINS_ETCD_JSONHELPER_H
#define PLUGINS_ETCD_JSONHELPER_H

#include "plugins/curl/HttpClient.h"

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <vector>

namespace plugins
{
namespace etcd
{

class JsonHelper
{
public:
    static void getSortedNodes(const std::string& json, std::vector<std::string>* nodes);
    static void getPostNode(const std::string& json, std::string* key, std::string* value);
};

} // etcd
} // JsonHelper

#endif // PLUGINS_ETCD_JSONHELPER_H