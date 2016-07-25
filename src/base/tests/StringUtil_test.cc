#include "base/StringUtil.h"
#include <stdio.h>

int main(int agrc, char* argv[])
{
	const char* values[] = { "111111", "222222", "333333", "444444", "555555", "666666" };
	for (size_t i = 0; i < sizeof(values)/sizeof(char*); i++)
	{
		fprintf(stdout, "%s: %u\n", values[i], base::StringUtil::hashCode(values[i]));
	}
	
	return 0;
}