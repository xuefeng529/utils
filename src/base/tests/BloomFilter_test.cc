#include "base/BloomFilter.h"
#include "base/Timestamp.h"

#include <iostream>

#include <stdio.h>

int main()
{
	std::vector<std::string> vals;
	std::vector<std::string> vals2;
	for (int i = 0; i < 10000000; i++)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "032%d", i);
		vals.push_back(buf);
		snprintf(buf, sizeof(buf), "032%d_", i);
		vals2.push_back(buf);
	}

	base::BloomFilter bf;
	for (size_t i = 0; i < vals.size(); i++)
	{
		bf.add(vals[i]);
	}
	
	size_t existedCount = 0;
	for (size_t i = 0; i < vals.size(); i++)
	{
		if (bf.mightContain(vals[i]))
		{
			existedCount++;
		}
	}

	std::cout << "existed count: " << existedCount << std::endl;

	size_t nonexistedCount = 0;
	for (size_t i = 0; i < vals2.size(); i++)
	{
		if (!bf.mightContain(vals2[i]))
		{
			nonexistedCount++;
		}
	}

	std::cout << "non existed count: " << nonexistedCount << std::endl;
}
