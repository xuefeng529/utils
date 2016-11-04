#include "plugins/openssl/Md5.h"
#include "base/StringUtil.h"

#include <openssl/md5.h>

#include <stdint.h>

namespace plugin
{

void OpenSSL::md5(const std::string& data, std::string* ret)
{
	MD5_CTX mdContext;
	MD5_Init(&mdContext);
	MD5_Update(&mdContext, data.data(), data.size());
	uint8_t buf[MD5_DIGEST_LENGTH];
	MD5_Final(buf, &mdContext);

	base::StringUtil::byteToHex(reinterpret_cast<char*>(buf), sizeof(buf), ret);
}

} // namespace plugin
