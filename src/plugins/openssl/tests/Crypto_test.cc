#include "plugins/openssl/Crypto.h"
#include "base/StringUtil.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		fprintf(stdout, "Usage: %s content key flag\n", argv[0]);
		abort();
	}
	
	if (atoi(argv[3]) == 1)
	{
		std::string ciphertext;
		plugin::openssl::Crypto::desEncrypt(argv[1], argv[2], &ciphertext);
		fprintf(stdout, "ciphertext: %s\n", ciphertext.c_str());
	}
	else
	{
		std::string plaintext;
		plugin::openssl::Crypto::desDecrypt(base::StringUtil::hexStrToByte(argv[1], strlen(argv[1])), argv[2], &plaintext);
		fprintf(stdout, "plaintext: %s\n", plaintext.c_str());
	}
	
	return 0;
}
