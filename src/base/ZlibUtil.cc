#include "base/ZlibUtil.h"
#include "base/Logging.h"

namespace base
{

bool ZlibUtil::compressData(const std::string& src, std::string* ret)
{
	return compressData(src.data(), src.size(), ret);
}

bool ZlibUtil::compressData(const char* src, size_t len, std::string* ret)
{
	uLong boundLen = compressBound(len);
	char* buf = new char[boundLen];
	if (buf == NULL)
	{
		LOG_ERROR << "no enough memory !";
		return false;
	}

	int code = compress(reinterpret_cast<Bytef*>(buf), &boundLen,
		reinterpret_cast<const Bytef*>(src), len);
	if (code != Z_OK)
	{
		LOG_ERROR << "compress failed: " << code;
        delete[] buf;
		return false;
	}

	ret->assign(buf, boundLen);
	delete[] buf;
	return true;
}

bool ZlibUtil::uncompressData(const char* src, size_t srcLen, char* dest, size_t* destLen)
{
	int code = uncompress(reinterpret_cast<Bytef*>(dest), reinterpret_cast<uLongf*>(destLen),
		reinterpret_cast<const Bytef*>(src), static_cast<uLong>(srcLen));
	if (code != Z_OK)
	{
		LOG_ERROR << "uncompress failed: " << code;
		return false;
	}

	return true;
}

} // namespace base
