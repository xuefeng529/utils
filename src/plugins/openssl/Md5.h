#ifndef PLUGIN_OPENSSL_MD5_H
#define PLUGIN_OPENSSL_MD5_H

#include <string>

namespace plugin
{
namespace openssl
{

class Md5
{
public:
	static std::string strToMd5(const std::string& str);
};

} // namespace openssl
} // namespace plugin

#endif // PLUGIN_OPENSSL_MD5_H
