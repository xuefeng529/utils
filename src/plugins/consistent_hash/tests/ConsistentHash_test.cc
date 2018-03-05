#include "plugins/consistent_hash/ConsistentHash.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	plugins::ConsistentHash consistentHash;
	for (int i = 0; i < 5; i++)
	{
		char name[64];
		snprintf(name, sizeof(name), "node_%d", i);
		consistentHash.addNode(name);
	}

	for (int i = 0; i < 10; i++)
	{
		char key[64];
        snprintf(key, sizeof(key), "key_%d", i);
        fprintf(stdout, "[%s] is in node: [%s]\n", key, consistentHash.lookup(key));
	}

	fprintf(stdout, "\n[node_0] is deleted.\n");
	consistentHash.deleteNode("node_0");
	for (int i = 0; i < 10; i++)
	{
        char key[64];
        snprintf(key, sizeof(key), "key_%d", i);
        fprintf(stdout, "[%s] is in node: [%s]\n", key, consistentHash.lookup(key));
	}

	fprintf(stdout, "\n[node_0] is added.\n");
	consistentHash.addNode("node_0");
	for (int i = 0; i < 10; i++)
	{
        char key[64];
        snprintf(key, sizeof(key), "key_%d", i);
        fprintf(stdout, "[%s] is in node: [%s]\n", key, consistentHash.lookup(key));
	}
	
    return 0;
}
