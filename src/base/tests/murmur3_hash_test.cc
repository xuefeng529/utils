#include "base/murmur3_hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
	char buf[512];
	while (fgets(buf, sizeof(buf), stdin) != NULL)
	{
		if (strcmp(buf, "quit") == 0)
		{
			break;
		}

		printf("hash: %u\n", base::murmurHash3(reinterpret_cast<const void*>(buf), strlen(buf)));
	}
	
	return 0;
}