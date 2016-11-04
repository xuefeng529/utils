#include "plugins/openssl/Md5.h"
#include "base/StringUtil.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	char buf[1024];
	while (fgets(buf, sizeof(buf), stdin) != NULL)
	{
		std::string md5;
		buf[strlen(buf) - 1] = '\0';
		plugin::OpenSSL::md5(buf, &md5);
		printf("md5: %s\n", md5.c_str());
	}
	
	return 0;
}
