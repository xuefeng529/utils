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
	if (src.empty())
	{
		return;
	}

	size_t pos = 0;
	size_t oldPos = 0;
	do
	{
		pos = src.find(sep, oldPos);
		if (pos != std::string::npos)
		{
			ret->push_back(src.substr(oldPos, pos - oldPos));
		}
		else
		{
			ret->push_back(src.substr(oldPos));
			break;
		}

		oldPos = pos + sep.size();
		if (oldPos == src.size())
		{
			ret->push_back(std::string());
		}
	} while (oldPos < src.size());
}

void StringUtil::replace(std::string* src, const std::string& sep, const std::string& str)
{
	for (size_t pos = src->find(sep); pos != std::string::npos; 
		pos = src->find(sep, pos + sep.size()))
	{
		src->replace(pos, sep.size(), str);
	}
}

void StringUtil::byteToHex(const char* src, size_t len, std::string* ret)
{
	static const char kHex[] = "0123456789ABCDEF";
	ret->clear();
	for (size_t i = 0; i < len; i++)
	{
		uint8_t c = src[i];
		ret->push_back(kHex[c >> 4]);
		ret->push_back(kHex[c & 0x0f]);
	}
}

void StringUtil::byteToHex(const std::string& src, std::string* ret)
{
	byteToHex(src.data(), src.size(), ret);
}

void StringUtil::hexToByte(const char* str, std::string* ret)
{
	size_t len = strlen(str);
	if (len == 0 || (len % 2) != 0)
	{
		return;
	}

	ret->clear();
	ret->resize(len / 2);
	uint8_t hex;
	uint8_t bin;
	for (size_t i = 0; i < len; i += 2)
	{
		hex = str[i];
		if (hex >= '0' && hex <= '9')
		{
			bin = (hex - '0') << 4;
		}
		else if (hex >= 'A' && hex <= 'F')
		{
			bin = (hex - 'A' + 10) << 4;
		}
		else if (hex >= 'a' && hex <= 'f')
		{
			bin = (hex - 'a' + 10) << 4;
		}
		else
		{
			return;
		}

		hex = str[i+1];
		if (hex >= '0' && hex <= '9')
		{
			bin |= (hex - '0');
		}
		else if (hex >= 'A' && hex <= 'F')
		{
			bin |= (hex - 'A' + 10);
		}
		else if (hex >= 'a' && hex <= 'f')
		{
			bin |= (hex - 'a' + 10);
		}
		else
		{
			return;
		}

		(*ret)[i/2] = bin;
	}
}

void StringUtil::hexToByte(const std::string& str, std::string* ret)
{
	hexToByte(str.c_str(), ret);
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

namespace{

const char HEX2DEC[256] =
{
	/*       0  1  2  3   4  5  6  7   8  9  A  B   C  D  E  F */
	/* 0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 1 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 2 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 3 */  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,

	/* 4 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 5 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 6 */ -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 7 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

	/* 8 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 9 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* A */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* B */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,

	/* C */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* D */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* E */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* F */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

// Only alphanum is safe.
const char SAFE[256] =
{
	/*      0 1 2 3  4 5 6 7  8 9 A B  C D E F */
	/* 0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 1 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 2 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,

	/* 4 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	/* 6 */ 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	/* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,

	/* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	/* C */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* D */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* E */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* F */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

}

void StringUtil::escape(const std::string& src, std::string* ret)
{
	ret->clear();
	if (src.empty())
	{
		return;
	}

	const char DEC2HEX[] = "0123456789ABCDEF";
	ret->resize(src.size() * 3, '\0');
	size_t i = 0;
	size_t j = 0;
	for (; i < src.size(); ++i)
	{
		if (SAFE[static_cast<uint8_t>(src[i])])
		{
			(*ret)[j++] = src[i];
		}
		else
		{
			(*ret)[j++] = '%';
			(*ret)[j++] = DEC2HEX[static_cast<uint8_t>(src[i]) >> 4];
			(*ret)[j++] = DEC2HEX[static_cast<uint8_t>(src[i]) & 0x0F];
		}
	}

	ret->resize(j);
}

void StringUtil::unescape(const std::string& src, std::string* ret)
{
	//for a esacep character, is %xx, min size is 3
	ret->clear();
	if (src.size() <= 2)
	{
		ret->append(src);
		return;
	}

	// Note from RFC1630:  "Sequences which start with a percent sign
	// but are not followed by two hexadecimal characters (0-9, A-F) are reserved
	// for future extension"

	ret->resize(src.size(), '\0');

	size_t i = 0;
	size_t j = 0;
	while (i < src.size() - 2)
	{
		if (src[i] == '%')
		{
			char dec1 = HEX2DEC[static_cast<uint8_t>(src[i + 1])];
			char dec2 = HEX2DEC[static_cast<uint8_t>(src[i + 2])];
			if (dec1 != -1 && dec2 != -1)
			{
				(*ret)[j++] = (dec1 << 4) + dec2;
				i += 3;
				continue;
			}
		}

		(*ret)[j++] = src[i++];
	}

	while (i < src.size())
	{
		(*ret)[j++] = src[i++];
	}

	ret->resize(j);
}

} // namespace base
