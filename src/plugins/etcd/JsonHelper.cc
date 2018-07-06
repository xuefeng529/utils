#include "plugins/etcd/JsonHelper.h"
#include "plugins/rapidjson/document.h"
#include "base/Logging.h"

namespace plugins
{
namespace etcd
{

void JsonHelper::getSortedNodes(const std::string& json, std::vector<std::string>* nodes)
{
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError())
    {
        LOG_ERROR << "parse failed: " << doc.GetParseError()
            << " json: " << json;
        return;
    }

    rapidjson::Value::ConstMemberIterator nodeIt = doc.FindMember("node");
    if (nodeIt == doc.MemberEnd())
    {
        LOG_ERROR << "node not found: " << json; 
        return;
    }
    
    rapidjson::Value::ConstMemberIterator nodesIt = nodeIt->value.FindMember("nodes");
    if (nodesIt != nodeIt->value.MemberEnd() && nodesIt->value.IsArray())
    {
        for (size_t n = 0; n < nodesIt->value.Size(); n++)
        {
            rapidjson::Value::ConstMemberIterator keyIt = nodesIt->value[n].FindMember("key");
            rapidjson::Value::ConstMemberIterator valIt = nodesIt->value[n].FindMember("value");
            if (keyIt != nodesIt->value[n].MemberEnd() && valIt != nodesIt->value[n].MemberEnd())
            {
                nodes->push_back(keyIt->value.GetString());
                nodes->push_back(valIt->value.GetString());
            }
        }
    }
}

void JsonHelper::getPostNode(const std::string& json, std::string* key, std::string* value)
{
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError())
    {
        LOG_ERROR << "parse failed: " << doc.GetParseError()
            << " json: " << json;
        return;
    }

    rapidjson::Value::ConstMemberIterator nodeIt = doc.FindMember("node");
    if (nodeIt == doc.MemberEnd())
    {
        LOG_ERROR << "node not found: " << json;
        return;
    }

    rapidjson::Value::ConstMemberIterator keyIt = nodeIt->value.FindMember("key");
    rapidjson::Value::ConstMemberIterator valueIt = nodeIt->value.FindMember("value");
    if (keyIt != nodeIt->value.MemberEnd() && valueIt != nodeIt->value.MemberEnd())
    {
        *key = keyIt->value.GetString();
        *value = valueIt->value.GetString();
    }
}

} // etcd
} // JsonHelper
