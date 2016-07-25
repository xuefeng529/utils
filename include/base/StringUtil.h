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
/// @source: ��Ҫ������ַ���
/// @c: �ָ��ַ�
/// @example:  ���strΪ��/usr/local/test/bin/������cΪ��/����
///            �����str��ɡ�/usr/local/test/bin��
void removeLast(std::string* source, char c);

/// ɾ���ַ���β����ָ���ַ�����ʼ������
/// @source: ��Ҫ������ַ���
/// @sep: �ָ��ַ���
/// @example: ���strΪ��/usr/local/test/bin/tt������sepΪ��/bin/����
///           �����str��ɡ�/usr/local/test
void removeLast(std::string* source, const std::string& sep);

/// ���ַ����е�����Сд�ַ�ת���ɴ�д
void toUpper(char* source);
void toUpper(std::string* source);

/// ���ַ����е����д�д�ַ�ת����Сд
void toLower(char* source);
void toLower(std::string* source);

/// ɾ���ַ�����β�ո��TAB��(\t)��س���(\r)���з�(\n)
void trim(char* source);
void trim(std::string* source);

/// ɾ���ַ����ײ��ո��TAB��(\t)��س���(\r)���з�(\n)
void trimLeft(char* source);
void trimLeft(std::string* source);

/// ɾ���ַ���β���ո��TAB��(\t)��س���(\r)���з�(\n)
void trimRight(char* source);
void trimRight(std::string* source);

/// ��string��delim�ָ���,�ŵ�list��
void split(const std::string& source, const std::string& sep, std::vector<std::string>* tokens);

/// hash_value
uint32_t hashCode(const char* str);
uint32_t hashCode(const std::string& str);

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

std::string extractDirname(const std::string& filepath);
std::string extractFilename(const std::string& filepath);
} // namespace StringUtil

} // namespace base

#endif // BASE_STRINGUTIL_H
