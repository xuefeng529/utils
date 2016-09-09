#include "plugins/openssl/Md5.h"
#include "base/StringUtil.h"

#include <iostream>

int main(int argc, char *argv[])
{
	if(argc > 1) 
	{
		std::string md5 = plugin::openssl::Md5::strToMd5(argv[1]);
		std::cout << "hex of md5: " << md5 << std::endl;
		std::string rawmd5 = base::StringUtil::hexStrToByte(md5);
		std::cout << "raw of md5: " << rawmd5 << std::endl;
		md5 = plugin::openssl::Md5::strToMd5(rawmd5);
		std::cout << "hex of rawmd5: " << md5 << std::endl;
	} 

	return 0;
}
