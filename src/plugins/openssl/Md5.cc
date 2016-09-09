#include "plugins/openssl/Md5.h"
#include "base/StringUtil.h"

#include<openssl/md5.h>

#include <stdint.h>

namespace plugin
{
namespace openssl
{

std::string Md5::strToMd5(const std::string& str)
{
	uint8_t buf[MD5_DIGEST_LENGTH];

	MD5_CTX mdContext;
	MD5_Init(&mdContext);
	MD5_Update(&mdContext, str.c_str(), str.size());
	MD5_Final(buf, &mdContext);

	return base::StringUtil::byteToHexStr(reinterpret_cast<char*>(buf), sizeof(buf));
}

} // namespace openssl
} // namespace plugin
