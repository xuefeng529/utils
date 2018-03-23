#ifndef PLUGINS_ZK_UTIL_H
#define PLUGINS_ZK_UTIL_H

#include <string>

#include <stdint.h>

namespace plugins
{
namespace zk
{

bool isChild(const std::string& child, const std::string& parent);
bool getParentPath(const std::string& path, std::string* parent);
const char * getNodeName(const std::string& path);
int32_t getSequenceNo(const std::string& name);
bool isValidPath(const std::string& path);

} // namespace zk
} // namespace plugins

#endif // PLUGINS_ZK_UTIL_H
