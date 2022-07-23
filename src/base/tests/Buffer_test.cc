#include "base/Buffer.h"

#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>
#include <stdio.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

class Uuid : public boost::uuids::uuid
{
public:
	Uuid() : uuid(g_rgen()) {}
	operator uuid() { return static_cast<uuid&>(*this); }
	operator uuid() const { return static_cast<const uuid&>(*this); }

	std::string get() const
	{
		return boost::uuids::to_string(*this);
	}

private:
	static boost::uuids::random_generator g_rgen;
};

boost::uuids::random_generator Uuid::g_rgen;

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

std::string genUuid()
{
	static boost::uuids::random_generator rgen;
	boost::uuids::uuid uuid = rgen(); 
	return boost::uuids::to_string(uuid);
}

int main(int argc, char* argv[])
{
	//boost::uuids::random_generator rgen;
	time_t begin = time(NULL);
	for (int i = 0; i < 3000000; i++)
	{
		//Uuid uuid;
		//uuid.get();
		//boost::uuids::uuid a_uuid = rgen(); 
		//boost::lexical_cast<std::string>(a_uuid);
		//std::string tmp_uuid = boost::uuids::to_string(a_uuid);
		genUuid();
		//std::cout << genUuid() << std::endl;
	}
	time_t end = time(NULL);
	std::cout << "take time: " << end - begin << std::endl;
	
	return 0;
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
