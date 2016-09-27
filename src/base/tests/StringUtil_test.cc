#include "base/StringUtil.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		fprintf(stdout, "Usage: %s content flag\n", argv[0]);
		abort();
	}

	if (atoi(argv[2]) == 1)
	{
		std::string hexStr = base::StringUtil::byteToHexStr(argv[1], strlen(argv[1]));
		fprintf(stdout, "To hex: %s\n", hexStr.c_str());
	}
	else
	{
		std::string byteStr = base::StringUtil::hexStrToByte(argv[1], strlen(argv[1]));
		fprintf(stdout, "To byte: %s\n", byteStr.c_str());
	}

	return 0;
	/*char byteBuf[] = { 0x9A, 0xBC };
	std::string hexStr = base::StringUtil::byteToHexStr(byteBuf, sizeof(byteBuf));
	fprintf(stdout, "%s\n", hexStr.c_str());
	std::string byteStr = base::StringUtil::hexStrToByte(hexStr.c_str(), hexStr.size());
	for (size_t i = 0; i < byteStr.size(); i++)
	{
		fprintf(stdout, "%02hhX", byteStr[i]);
	}

	fprintf(stdout, "\n");
	return 0;
	const char* values[] = { "111111", "222222", "333333", "444444", "555555", "666666" };
	for (size_t i = 0; i < sizeof(values)/sizeof(char*); i++)
	{
		fprintf(stdout, "%s: %u\n", values[i], base::StringUtil::hashCode(values[i]));
	}
	
	return 0;*/
}