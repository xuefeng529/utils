#ifndef BASE_ZLIBUTIL_H
#define BASE_ZLIBUTIL_H

#include <string>

#include <zlib.h>

namespace base
{

class ZlibUtil
{
public:
    static bool compressData(const std::string& src, std::string* ret);
    static bool compressData(const char* src, size_t len, std::string* ret);
    static bool uncompressData(const char* src, size_t srcLen, char* dest, size_t* destLen);
};

} // namespace base

#endif // BASE_ZLIBUTIL_H
