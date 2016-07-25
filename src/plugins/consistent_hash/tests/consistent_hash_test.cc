#include "plugins/consistent_hash/ConsistentHash.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	plugin::hash::ConsistentHash consistentHash;
	for (int i = 0; i < 5; i++)
	{
		char name[64];
		snprintf(name, sizeof(name), "172.16.40.18%d", i);
		consistentHash.addNode(name);
	}

	for (int i = 0; i < 10; i++)
	{
		char name[64];
		snprintf(name, sizeof(name), "172.16.40.1%d", i);
		fprintf(stdout, "[%s] is in node: [%s]\n", name, consistentHash.lookup(name));
	}

	fprintf(stdout, "\n[172.16.40.180] is deleted.\n");
	consistentHash.deleteNode("172.16.40.180");
	for (int i = 0; i < 10; i++)
	{
		char name[64];
		snprintf(name, sizeof(name), "172.16.40.1%d", i);
		fprintf(stdout, "[%s] is in node: [%s]\n", name, consistentHash.lookup(name));
	}

	fprintf(stdout, "\n[172.16.40.185] is added.\n");
	consistentHash.addNode("172.16.40.185");
	for (int i = 0; i < 10; i++)
	{
		char name[64];
		snprintf(name, sizeof(name), "172.16.40.1%d", i);
		fprintf(stdout, "[%s] is in node: [%s]\n", name, consistentHash.lookup(name));
	}
	
    return 0;
}
