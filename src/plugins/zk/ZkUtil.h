#ifndef PLUGIN_ZK_ZKUTIL_H
#define PLUGIN_ZK_ZKUTIL_H

#include <string>

#include <stdint.h>

namespace zk
{
namespace ZkUtil 
{

bool isChild(const std::string& child, const std::string& parent);
bool getParentPath(const std::string& path, std::string* parent);
const char * getNodeName(const std::string& path);
int32_t getSequenceNo(const std::string& name);
bool isValidPath(const std::string& path);

} // namespace ZkUtil 
} // namespace zk

#endif // PLUGIN_ZK_ZKUTIL_H
