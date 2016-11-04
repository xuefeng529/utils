#ifndef PLUGIN_OPENSSL_CRYPTO_H
#define PLUGIN_OPENSSL_CRYPTO_H

#include <string>

namespace plugin
{
namespace OpenSSL
{
	
bool desEncrypt(const std::string& plaintext, const std::string& key, std::string* ciphertext);
bool desDecrypt(const std::string& ciphertext, const std::string& key, std::string* plaintext);

} // namespace OpenSSL
} // namespace plugin

#endif // PLUGIN_OPENSSL_CRYPTO_H
