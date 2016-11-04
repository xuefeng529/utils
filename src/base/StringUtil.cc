#include "StringUtil.h"

#include <algorithm>

#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

namespace base
{

void StringUtil::removeLast(std::string* src, char c)
{
	std::string::size_type pos = src->rfind(c);
	if (pos + 1 == src->length())
	{
		src->erase(pos);
	}
}

void StringUtil::removeLast(std::string* src, const std::string& sep)
{
	size_t pos = src->rfind(sep);
	if (pos != std::string::npos)
	{
		src->erase(pos);
	}
}

void StringUtil::toUpper(char* src)
{
    char* p = src;
	while (*p != '\0')
    {
		if ((*p >= 'a') && (*p <= 'z'))
		{
			*p += 'A' - 'a';
		}
		++p;
    }
}

void StringUtil::toLower(char* src)
{
	char* p = src;
	while (*p != '\0')
    {
		if ((*p >= 'A') && (*p <= 'Z'))
		{
			*p += 'a' - 'A';
		}
		++p;
    }
}

void StringUtil::toUpper(std::string* src)
{
	/// 只修改大小写，可以这样做
	char* p = const_cast<char*>(src->c_str());
	toUpper(p);
}

void StringUtil::toLower(std::string* src)
{
	char* p = const_cast<char*>(src->c_str());
    toLower(p);
}

void StringUtil::trimLeft(char* src)
{
    char* p = src;
	while (*p != '\0' && isspace(*p))
	{
		++p;
	}

	if (p == src)
	{
		return;
	}

	size_t remain = strlen(p) + 1;
	memmove(src, p, remain);
}

void StringUtil::trimRight(char* src)
{
	size_t len = strlen(src);
	if (len == 0)
	{
		return;
	}

	char* end = src + len - 1;
	char* p = end;
	while (p >= src && isspace(*p))
	{
		p--;
	}

	if (p != end)
	{
		*(p + 1) = '\0';
	}
}

void StringUtil::trim(char* src)
{
	trimLeft(src);
	trimRight(src);
}

void StringUtil::trimLeft(std::string* src)
{
    /// 不能直接对c_str()进行修改，因为长度发生了变化
    size_t length = src->length();
    char* tmp = new char[length+1];
    strncpy(tmp, src->c_str(), length);
    tmp[length] = '\0';
    trimLeft(tmp);
    *src = tmp;
	delete[] tmp;
}

void StringUtil::trimRight(std::string* src)
{
    /// 不能直接对c_str()进行修改，因为长度发生了变
    size_t length = src->length();
    char* tmp = new char[length+1];
	strncpy(tmp, src->c_str(), length);
	tmp[length] = '\0';
	trimRight(tmp);
	*src = tmp;
	delete[] tmp;
}

void StringUtil::trim(std::string* src)
{
	trimLeft(src);
	trimRight(src);
}

bool StringUtil::equal(const std::string& str1, const std::string& str2, bool nocase)
{
	if (nocase)
	{
		return (strncasecmp(str1.c_str(), str2.c_str(), str2.length()) == 0);
	}
	else
	{
		return (strncmp(str1.c_str(), str2.c_str(), str2.length()) == 0);
	}
}

void StringUtil::split(const std::string& src, const std::string& sep, std::vector<std::string>* ret)
{
	ret->clear();
	size_t pos = 0;
	size_t oldPos = 0;
	do
	{
		pos = src.find(sep, oldPos);
		if (pos == oldPos)
		{
			oldPos = pos + 1;
		}
		else if (pos == std::string::npos)
		{
			ret->push_back(src.substr(oldPos));
			break;
		}
		else
		{
			ret->push_back(src.substr(oldPos, pos - oldPos));
			oldPos = pos + 1;
		}
		oldPos = src.find_first_not_of(sep, oldPos);
	} while (pos != std::string::npos && oldPos != std::string::npos);
}

void StringUtil::replace(std::string* src, const std::string& sub, const std::string& str)
{
	for (size_t pos = src->find(sub); pos != std::string::npos; pos = src->find(sub))
	{
		src->replace(pos, sub.size(), str);
	}
}

void StringUtil::byteToHex(const char* src, size_t len, std::string* ret)
{
	ret->clear();
	char buf[3];
	for (size_t i = 0; i < len; i++)
	{
		snprintf(buf, sizeof(buf), "%02X", static_cast<uint8_t>(src[i]));
		ret->append(buf);
	}
}

void StringUtil::byteToHex(const std::string& src, std::string* ret)
{
	byteToHex(src.data(), src.size(), ret);
}

void StringUtil::hexToByte(const char* str, size_t len, std::string* ret)
{
	if (len == 0 || (len%2) != 0)
	{
		return;
	}
	
	ret->clear();
	ret->resize(len / 2);
	for (size_t i = 0; i < len; i+=2)
	{
		sscanf(&str[i], "%2hhX", const_cast<char*>(ret->data()) + i / 2);
	}
}

void StringUtil::hexToByte(const std::string& str, std::string* ret)
{
	hexToByte(str.data(), str.size(), ret);
}

uint64_t StringUtil::hash(const char* str)
{
	uint64_t ret = 0;
	while (*str != '\0')
	{
		ret = ret * 131 + (*str++);
	}
	return ret;
}

uint64_t StringUtil::hash(const std::string& str)
{
	return hash(str.c_str());
}

std::string StringUtil::int32ToStr(int32_t v)
{
	char buf[32] = { 0 };
	snprintf(buf, sizeof(buf), "%" PRId32 "", v);
	return std::string(buf);
}

std::string StringUtil::uint32ToStr(uint32_t v)
{
	char buf[32] = { 0 };
	snprintf(buf, sizeof(buf), "%" PRIu32 "", v);
	return std::string(buf);
}

std::string StringUtil::int64ToStr(int64_t v)
{
	char buf[32] = { 0 };
	snprintf(buf, sizeof(buf), "%" PRId64 "", v);
	return std::string(buf);
}

std::string StringUtil::uint64ToStr(uint64_t v)
{
	char buf[32] = { 0 };
	snprintf(buf, sizeof(buf), "%" PRIu64 "", v);
	return std::string(buf);
}

std::string StringUtil::floatToStr(float v)
{
	return doubleToStr(static_cast<double>(v));
}

std::string StringUtil::doubleToStr(double v)
{
	char buf[21] = { 0 };
	if (v - floor(v) == 0)
	{
		snprintf(buf, sizeof(buf), "%.0f", v);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%f", v);
	}
	return std::string(buf);
}

int32_t StringUtil::strToInt32(const std::string& str)
{
	const char *start = str.c_str();
	char *end;
	int32_t ret = static_cast<int32_t>(strtol(start, &end, 10));
	if (*end == '\0' && size_t(end - start) == str.size())
	{
		errno = 0;
	}
	else
	{
		errno = EINVAL;
	}
	return ret;
}

uint32_t StringUtil::strToUInt32(const std::string& str)
{
	const char *start = str.c_str();
	char *end;
	uint32_t ret = static_cast<uint32_t>(strtoul(start, &end, 10));
	if (*end == '\0' && size_t(end - start) == str.size())
	{
		errno = 0;
	}
	else
	{
		errno = EINVAL;
	}
	return ret;
}

int64_t StringUtil::strToInt64(const std::string& str)
{
	const char *start = str.c_str();
	char *end;
	int64_t ret = static_cast<int64_t>(strtoll(start, &end, 10));
	if (*end == '\0' && size_t(end - start) == str.size())
	{
		errno = 0;
	}
	else
	{
		errno = EINVAL;
	}
	return ret;
}

uint64_t StringUtil::strToUInt64(const std::string& str)
{
	const char *start = str.c_str();
	char *end;
	uint64_t ret = static_cast<uint64_t>(strtoull(start, &end, 10));
	if (*end == '\0' && size_t(end - start) == str.size())
	{
		errno = 0;
	}
	else
	{
		errno = EINVAL;
	}
	return ret;
}

float StringUtil::strToFloat(const std::string& str)
{
	const char *start = str.c_str();
	char *end;
	float ret = static_cast<float>(strtof(start, &end));
	if (*end == '\0' && size_t(end - start) == str.size())
	{
		errno = 0;
	}
	else
	{
		errno = EINVAL;
	}
	return ret;
}

double StringUtil::strToDouble(const std::string& str)
{
	const char *start = str.c_str();
	char *end;
	double ret = static_cast<double>(strtod(start, &end));
	if (*end == '\0' && size_t(end - start) == str.size())
	{
		errno = 0;
	}
	else
	{
		errno = EINVAL;
	}
	return ret;
}

std::string StringUtil::extractDirname(const std::string& filepath)
{
	std::string dirpath;
	const char* pos = strrchr(filepath.c_str(), '/');
	if (NULL == pos)
	{
		dirpath = filepath;
	}
	else
	{
		dirpath.assign(filepath.c_str(), pos - filepath.c_str());
	}
	return dirpath;
}

std::string StringUtil::extractFilename(const std::string& filepath)
{
	std::string filename;
	const char* pos = strrchr(filepath.c_str(), '/');
	if (NULL == pos)
	{
		filename = filepath;
	}
	else
	{
		filename.assign(pos + 1);
	}
	return filename;
}

} // namespace base
