#include "base/StringUtil.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
	std::string orig = "http://172.16.56.27:9696/UserCfgSet.aspx?a=u0321011&b=1,600002&c=2&d=2&e=2&f=&g=10&h=5&i=20130903&j=20130903&k=1&l=123";
	std::string escaped;
	base::StringUtil::escape(orig, &escaped);
	printf("escape: %s\n", escaped.c_str());

	std::string unescaped;
	base::StringUtil::unescape(escaped, &unescaped);
	printf("unescape: %s\n", unescaped.c_str());
	if (orig == unescaped)
	{
		printf("it is ok\n");
	}
	else
	{
		printf("it is not ok\n");
	}
	return 0;
	if (argc < 3)
	{
		fprintf(stdout, "Usage: %s content flag\n", argv[0]);
		abort();
	}

	if (atoi(argv[2]) == 1)
	{
		/*std::string hexStr;
		base::StringUtil::byteToHex(argv[1], strlen(argv[1]), &hexStr);
		fprintf(stdout, "To hex: %s\n", hexStr.c_str());*/
		std::string escaped;
		base::StringUtil::escape("http://172.16.56.27:9696/UserCfgSet.aspx?a=u0321011&b=1,600002&c=2&d=2&e=2&f=&g=10&h=5&i=20130903&j=20130903&k=1&l=123", &escaped);
		printf("escape: %s\n", escaped.c_str());
	}
	else
	{
		/*std::string byteStr;
		base::StringUtil::hexToByte(argv[1], &byteStr);
		fprintf(stdout, "To byte: %s\n", byteStr.c_str());*/
		std::string unescaped;
		base::StringUtil::unescape(argv[1], &unescaped);
		printf("unescape: %s\n", unescaped.c_str());
	}

	return 0;
}