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

/// ɾ���ַ���β����ָ���ַ���ʼ������
/// @src: ��Ҫ������ַ���
/// @c: �ָ��ַ�
/// @example: ���srcΪ��/usr/local/test/bin/������cΪ��/����
///           �����src��ɡ�/usr/local/test/bin��
void removeLast(std::string* src, char c);

/// ɾ���ַ���β����ָ���ַ�����ʼ������
/// @src: ��Ҫ������ַ���
/// @sep: �ָ��ַ���
/// @example: ���srcΪ��/usr/local/test/bin/tt������sepΪ��/bin/����
///           �����src��ɡ�/usr/local/test
void removeLast(std::string* src, const std::string& sep);

/// ���ַ����е�����Сд�ַ�ת���ɴ�д
void toUpper(char* src);
void toUpper(std::string* src);

/// ���ַ����е����д�д�ַ�ת����Сд
void toLower(char* src);
void toLower(std::string* src);

/// ɾ���ַ�����β�ո��TAB��(\t)��س���(\r)���з�(\n)
void trim(char* src);
void trim(std::string* src);

/// ɾ���ַ����ײ��ո��TAB��(\t)��س���(\r)���з�(\n)
void trimLeft(char* src);
void trimLeft(std::string* src);

/// ɾ���ַ���β���ո��TAB��(\t)��س���(\r)���з�(\n)
void trimRight(char* src);
void trimRight(std::string* src);

/// �Ƚ������ַ����Ƿ����
bool equal(const std::string& str1, const std::string& str2, bool nocase);

/// ��string��delim�ָ���,�ŵ�list��
void split(const std::string& src, const std::string& sep, std::vector<std::string>* ret);

/// �滻�ַ�����ָ�����Ӵ�
/// @src: Դ�ַ���
/// @sub: ���滻���ַ���
/// @str: �滻���ַ���
void replace(std::string* src, const std::string& sep, const std::string& str);

/// �ֽ���ת����ʮ�������ַ���
void byteToHex(const char* src, size_t len, std::string* ret);
void byteToHex(const std::string& src, std::string* ret);

/// ʮ�������ַ���ת�����ֽ���
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
