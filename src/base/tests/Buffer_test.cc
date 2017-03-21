#include "base/Buffer.h"

#include <stdint.h>
#include <stdio.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

void test()
{
	std::string ss(10, '\0');
	printf("%d\n", static_cast<int>(ss.size()));
	const std::string kToken = "token";
	const std::string kJson = "{a: 'Hello', b: 'World'}";
	base::Buffer buf;
	buf.appendInt16(123);
	buf.appendInt32(kToken.size());
	buf.append(kToken);
	buf.appendInt32(kJson.size());
	buf.append(kJson);
	std::string val = buf.retrieveAllAsString();
	printf("length of buf: %d\n", static_cast<int>(val.size()));
}

int main(int argc, char* argv[])
{
	test();
	/*base::Buffer buf;
	buf.appendInt16(16);
	buf.appendInt32(32);
	buf.appendInt64(64);
	const char* data = "{a: 'Hello', b: 'World'}";
	buf.appendInt32(strlen(data));
	buf.append(data);
	int16_t n16 = buf.readInt16();
	int32_t n32 = buf.readInt32();
	int64_t n64 = buf.readInt64();
	int32_t len = buf.readInt32();
	std::string str = buf.retrieveAsString(len);
	printf("%d %d %" PRId64 " %d %s\n", n16, n32, n64, len, str.c_str());*/
	return 0;
}