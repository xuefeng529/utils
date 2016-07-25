#include "base/crc32c.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

void test1()
{
	char buf[32];

	memset(buf, 0, sizeof(buf));
	assert(0x8a9136aa == base::crc32c::value(buf, sizeof(buf)));

	memset(buf, 0xff, sizeof(buf));
	assert(0x62a8ab43 == base::crc32c::value(buf, sizeof(buf)));

	for (int i = 0; i < 32; i++) {
		buf[i] = i;
	}
	assert(0x46dd794e == base::crc32c::value(buf, sizeof(buf)));

	for (int i = 0; i < 32; i++) {
		buf[i] = 31 - i;
	}
	assert(0x113fdb5c == base::crc32c::value(buf, sizeof(buf)));

	unsigned char data[48] = {
		0x01, 0xc0, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x14, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x04, 0x00,
		0x00, 0x00, 0x00, 0x14,
		0x00, 0x00, 0x00, 0x18,
		0x28, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x02, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
	};
	assert(0xd9963a56 == base::crc32c::value(reinterpret_cast<char*>(data), sizeof(data)));
}

void test2()
{
	assert(base::crc32c::value("a", 1) != base::crc32c::value("foo", 3));
}

void test3()
{
	assert(base::crc32c::value("hello world", 11) == 
		base::crc32c::extend(base::crc32c::value("hello ", 6), "world", 5));
}

void test4()
{
	uint32_t crc = base::crc32c::value("foo", 3);
	assert(crc != base::crc32c::mask(crc));
	assert(crc != base::crc32c::mask(base::crc32c::mask(crc)));
	assert(crc == base::crc32c::unmask(base::crc32c::mask(crc)));
	assert(crc == base::crc32c::unmask(base::crc32c::unmask(base::crc32c::mask(base::crc32c::mask(crc)))));
}

int main(int argc, char* argv[])
{
	test1();
	test2();
	test3();
	test4();
	return 0;
}