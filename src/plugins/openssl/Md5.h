#ifndef PLUGIN_OPENSSL_MD5_H
#define PLUGIN_OPENSSL_MD5_H

#include <string>

namespace plugin
{
namespace OpenSSL
{

void md5(const std::string& data, std::string* ret);

} // namespace OpenSSL
} // namespace plugin

#endif // PLUGIN_OPENSSL_MD5_H
