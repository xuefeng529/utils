#include "StringUtil.h"

#include <algorithm>

#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

namespace base
{

void StringUtil::removeLast(std::string* source, char c)
{
	std::string::size_type pos = source->rfind(c);
	if (pos + 1 == source->length())
	{
		source->erase(pos);
	}
}

void StringUtil::removeLast(std::string* source, const std::string& sep)
{
	std::string::size_type pos = source->rfind(sep);
	if (pos != std::string::npos)
	{
		source->erase(pos);
	}
}

void StringUtil::toUpper(char* source)
{
    char* p = source;
	while (*p != '\0')
    {
		if ((*p >= 'a') && (*p <= 'z'))
		{
			*p += 'A' - 'a';
		}
		++p;
    }
}

void StringUtil::toLower(char* source)
{
	char* p = source;
	while (*p != '\0')
    {
		if ((*p >= 'A') && (*p <= 'Z'))
		{
			*p += 'a' - 'A';
		}
		++p;
    }
}

void StringUtil::toUpper(std::string* source)
{
	/// 只修改大小写，可以这样做
	char* p = const_cast<char*>(source->c_str());
	toUpper(p);
}

void StringUtil::toLower(std::string* source)
{
	char* p = const_cast<char*>(source->c_str());
    toLower(p);
}

void StringUtil::trim(char* source)
{
    char* space = NULL;
    char* p = source;
	while (' ' == *p)
	{
		++p;
	}

    for (;;)
    {
        *source = *p;
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
				space = source;
			}
        }
        else
        {
            space = NULL;
        }

        ++source;
        ++p;
    }
}

void StringUtil::trimLeft(char* source)
{
    char* p = source;
	while (isspace(*p))
	{
		++p;
	}

    for (;;)
    {
        *source = *p;
		if ('\0' == *p)
		{
			break;
		}
        ++source;
        ++p;
    }
}

void StringUtil::trimRight(char* source)
{
    char* space = NULL;
    char* p = source;
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

void StringUtil::trim(std::string* source)
{
    trimLeft(source);
    trimRight(source);
}

void StringUtil::trimLeft(std::string* source)
{
    /// 不能直接对c_str()进行修改，因为长度发生了变化
    size_t length = source->length();
    char* tmp = new char[length+1];
    strncpy(tmp, source->c_str(), length);
    tmp[length] = '\0';
    trimLeft(tmp);
    *source = tmp;
	delete[] tmp;
}

void StringUtil::trimRight(std::string* source)
{
    /// 不能直接对c_str()进行修改，因为长度发生了变
    size_t length = source->length();
    char* tmp = new char[length+1];
	strncpy(tmp, source->c_str(), length);
	tmp[length] = '\0';
	trimRight(tmp);
	*source = tmp;
	delete[] tmp;
}

void StringUtil::split(const std::string& source, const std::string& sep, std::vector<std::string>* tokens)
{
	tokens->clear();
	std::string::size_type pos = 0;
	std::string::size_type oldPos = 0;
	do
	{
		pos = source.find(sep, oldPos);
		if (pos == oldPos)
		{
			oldPos = pos + 1;
		}
		else if (pos == std::string::npos)
		{
			tokens->push_back(source.substr(oldPos));
			break;
		}
		else
		{
			tokens->push_back(source.substr(oldPos, pos - oldPos));
			oldPos = pos + 1;
		}
		oldPos = source.find_first_not_of(sep, oldPos);
	} while (pos != std::string::npos && oldPos != std::string::npos);
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
		if (errno == 0)
		{
			errno = EINVAL;
		}
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
		if (errno == 0)
		{
			errno = EINVAL;
		}
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
		if (errno == 0)
		{
			errno = EINVAL;
		}
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
		if (errno == 0)
		{
			errno = EINVAL;
		}
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
		if (errno == 0)
		{
			errno = EINVAL;
		}
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
		if (errno == 0)
		{
			errno = EINVAL;
		}
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
