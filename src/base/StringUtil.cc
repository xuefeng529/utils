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
	std::string::size_type pos = src->rfind(sep);
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

void StringUtil::trim(char* src)
{
    char* space = NULL;
    char* p = src;
	while (' ' == *p)
	{
		++p;
	}

    for (;;)
    {
        *src = *p;
        if ('\0' == *p)
        {
			if (space != NULL)
			{
				*space = '\0';
			}
            break;
        }
        else if (isspace(*p))
        {
			if (NULL == space)
			{
				space = src;
			}
        }
        else
        {
            space = NULL;
        }

        ++src;
        ++p;
    }
}

void StringUtil::trimLeft(char* src)
{
    char* p = src;
	while (isspace(*p))
	{
		++p;
	}

    for (;;)
    {
        *src = *p;
		if ('\0' == *p)
		{
			break;
		}
        ++src;
        ++p;
    }
}

void StringUtil::trimRight(char* src)
{
    char* space = NULL;
    char* p = src;
    for (;;)
    {
        if ('\0' == *p)
        {
			if (space != NULL)
			{
				*space = '\0';
			}
            break;
        }
        else if (isspace(*p))
        {
			if (NULL == space)
			{
				space = p;
			}
        }
        else
        {
            space = NULL;
        }

        ++p;
    }
}

void StringUtil::trim(std::string* src)
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

void StringUtil::split(const std::string& src, const std::string& sep, std::vector<std::string>* tokens)
{
	tokens->clear();
	std::string::size_type pos = 0;
	std::string::size_type oldPos = 0;
	do
	{
		pos = src.find(sep, oldPos);
		if (pos == oldPos)
		{
			oldPos = pos + 1;
		}
		else if (pos == std::string::npos)
		{
			tokens->push_back(src.substr(oldPos));
			break;
		}
		else
		{
			tokens->push_back(src.substr(oldPos, pos - oldPos));
			oldPos = pos + 1;
		}
		oldPos = src.find_first_not_of(sep, oldPos);
	} while (pos != std::string::npos && oldPos != std::string::npos);
}

std::string StringUtil::byteToHexStr(const char* src, size_t len)
{
	std::string ret;
	char buf[3];
	for (size_t i = 0; i < len; i++)
	{
		snprintf(buf, sizeof(buf), "%02X", static_cast<uint8_t>(src[i]));
		ret.append(buf);
	}
	return ret;
}

std::string StringUtil::hexStrToByte(const char* str, size_t len)
{
	std::string ret;
	if (len == 0 || (len%2) != 0)
	{
		return ret;
	}
	
	ret.resize(len / 2);
	for (size_t i = 0; i < len; i+=2)
	{
		sscanf(&str[i], "%2hhX", &*ret.begin() + i / 2);
	}

	return ret;
}

std::string StringUtil::hexStrToByte(const std::string& str)
{
	return hexStrToByte(str.c_str(), str.size());
}

uint32_t StringUtil::hashCode(const char* str)
{
	uint32_t g;
	uint32_t h = 0;
	const char *p = str;
	size_t len = strlen(str);
	while (p < str + len)
	{
		h = (h << 4) + *p++;
		if ((g = (h & 0xF0000000)))
		{
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}

	return h;
}

uint32_t StringUtil::hashCode(const std::string& str)
{
	return hashCode(str.c_str());
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
