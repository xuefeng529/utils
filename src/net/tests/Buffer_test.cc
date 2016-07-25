#include "net/Buffer.h"

#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

const char* url = "GET /UserCfgGet.aspx?a=6&b=1,600002 HTTP/1.0\r\n"
"Accept: */*\r\n"
"Connection: Keep - Alive\r\n"
"Host: 172.16.56.27:9898\r\n"
"User-Agent: ApacheBench/2.3\r\n\r\n";

void test()
{
	net::Buffer bufffer;
	bufffer.appendInt32(123);
	bufffer.appendInt8(static_cast<int8_t>(strlen("xuefeng")));
	bufffer.append("xuefeng");
	bufffer.appendInt8(28);

	int32_t id = bufffer.readInt32();
	std::string name;
	bufffer.retrieveAsString(bufffer.readInt8(), &name);
	int8_t age = bufffer.readInt8();
	fprintf(stdout, "id: %d, name: %s, age: %d\n", id, name.c_str(), age);
}

void testLine()
{
	net::Buffer bufffer;
	bufffer.append(url);
	std::string line;
	bool f;
	while ((f = bufffer.readLine(&line)))
	{
		fprintf(stdout, "%s\n", line.c_str());
	}
}

void testRemove()
{
	for (int i = 0; i < 100000; i++)
	{
		net::Buffer dst;
		size_t len;

		{
			net::Buffer src;
			src.appendInt32(123);
			src.appendInt8(static_cast<int8_t>(strlen("xuefeng")));
			src.append("xuefeng");
			src.appendInt8(28);
			len = src.length();
			fprintf(stdout, "src = %" PRIu64 "\n", len);
			dst.removeBuffer(&src);
			len = src.length();
		}

		fprintf(stdout, "src = %" PRIu64 ", dst = %" PRIu64 "\n", len, dst.length());
		int32_t id = dst.readInt32();
		int8_t nameLen = dst.readInt8();
		char* name = new char[nameLen + 1];
		memset(name, 0, nameLen + 1);
		dst.retrieveAsBytes(name, nameLen);
		int8_t age = dst.readInt8();
		fprintf(stdout, "id: %d, name[bytes]: %s, age: %d\n", id, name, age);
		delete[] name;
	}
}

void testAdd()
{
	for (int i = 0; i < 100000; i++)
	{
		net::Buffer dst;
		size_t len;

		{
			net::Buffer src;
			src.appendInt32(123);
			src.appendInt8(static_cast<int8_t>(strlen("xuefeng")));
			src.append("xuefeng");
			src.appendInt8(28);
			len = src.length();
			fprintf(stdout, "src = %" PRIu64 "\n", len);
			dst.appendBuffer(src);
			len = src.length();
		}

		fprintf(stdout, "src = %" PRIu64 ", dst = %" PRIu64 "\n", len, dst.length());
		int32_t id = dst.readInt32();
		std::string name;
		dst.retrieveAsString(dst.readInt8(), &name);
		int8_t age = dst.readInt8();
		fprintf(stdout, "id: %d, name: %s, age: %d\n", id, name.c_str(), age);
	}
}

int main()
{
	test();
	testLine();
	testRemove();
	testAdd();
	return 0;
}