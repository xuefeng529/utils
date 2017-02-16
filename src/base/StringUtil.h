#ifndef BASE_STRINGUTIL_H
#define BASE_STRINGUTIL_H

#include <string>
#include <vector>
#include <sstream>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

namespace base
{
namespace StringUtil
{

/// 删除字符串尾部从指定字符开始的内容
/// @src: 需要处理的字符串
/// @c: 分隔字符
/// @example: 如果src为“/usr/local/test/bin/”，而c为“/”，
///           则处理后src变成“/usr/local/test/bin”
void removeLast(std::string* src, char c);

/// 删除字符串尾部从指定字符串开始的内容
/// @src: 需要处理的字符串
/// @sep: 分隔字符串
/// @example: 如果src为“/usr/local/test/bin/tt”，而sep为“/bin/”，
///           则处理后src变成“/usr/local/test
void removeLast(std::string* src, const std::string& sep);

/// 将字符串中的所有小写字符转换成大写
void toUpper(char* src);
void toUpper(std::string* src);

/// 将字符串中的所有大写字符转换成小写
void toLower(char* src);
void toLower(std::string* src);

/// 删除字符串首尾空格或TAB符(\t)或回车符(\r)或换行符(\n)
void trim(char* src);
void trim(std::string* src);

/// 删除字符串首部空格或TAB符(\t)或回车符(\r)或换行符(\n)
void trimLeft(char* src);
void trimLeft(std::string* src);

/// 删除字符串尾部空格或TAB符(\t)或回车符(\r)或换行符(\n)
void trimRight(char* src);
void trimRight(std::string* src);

/// 比较两个字符串是否相等
bool equal(const std::string& str1, const std::string& str2, bool nocase);

/// 把string以delim分隔开,放到list中
void split(const std::string& src, const std::string& sep, std::vector<std::string>* ret);

/// 替换字符串中指定的子串
/// @src: 源字符串
/// @sub: 被替换的字符串
/// @str: 替换的字符串
void replace(std::string* src, const std::string& sep, const std::string& str);

/// 字节流转换成十六进制字符串
void byteToHex(const char* src, size_t len, std::string* ret);
void byteToHex(const std::string& src, std::string* ret);

/// 十六进制字符串转换成字节流
void hexToByte(const char* str, std::string* ret);
void hexToByte(const std::string& str, std::string* ret);

/// bkdrhash function
uint64_t hash(const char* str);
uint64_t hash(const std::string& str);

std::string int32ToStr(int32_t v);
std::string uint32ToStr(uint32_t v);
std::string int64ToStr(int64_t v);
std::string uint64ToStr(uint64_t v);
std::string floatToStr(float v);
std::string doubleToStr(double v);

int32_t strToInt32(const std::string& str);
uint32_t strToUInt32(const std::string& str);
int64_t strToInt64(const std::string& str);
uint64_t strToUInt64(const std::string& str);
float strToFloat(const std::string& str);
double strToDouble(const std::string& str);

void unescape(const std::string& str, std::string* ret);

std::string extractDirname(const std::string& filepath);
std::string extractFilename(const std::string& filepath);

} // namespace StringUtil
} // namespace base

#endif // BASE_STRINGUTIL_H
