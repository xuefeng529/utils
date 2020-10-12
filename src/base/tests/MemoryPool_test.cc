#include "base/MemoryPool.h"
#include "base/Mutex.h"
#include "base/Timestamp.h"

#include <iostream>

#include <string.h>

struct Block
{
	int i_;
	float f_;

	Block(int i, float f)
		: i_(i), f_(f)
	{
		//std::cout << "Block" << std::endl;
	}

	~Block() 
	{
		//std::cout << "~Block" << std::endl;
	}

	void* operator new(size_t n)
	{
		return base::MTObjectPool<Block, base::MutexLock>::malloc();
	}

	void operator delete(void* ptr, size_t n)
	{
		base::MTObjectPool<Block, base::MutexLock>::free(reinterpret_cast<Block*>(ptr));
	}
};

int main(int argc, char* argv[])
{
	Block* blocks[1000];
	int64_t begin = base::Timestamp::now().microSecondsSinceEpoch();
	for (int n = 0; n < 1000; n++)
	{
		for (int i = 0; i < 1000; i++)
		{
			blocks[i] =  new Block(i, 3.14f);
		}

		if (n == 999)
		{
			std::cout << blocks[999]->i_ << " " << blocks[999]->f_ << std::endl;
		}
		
		for (int i = 0; i < 1000; i++)
		{
			delete blocks[i];
		}
	}
	
	int64_t end = base::Timestamp::now().microSecondsSinceEpoch();
	std::cout << "take time: " << end - begin << "[us]" << std::endl;
	std::string line;
	std::getline(std::cin, line);
	return 0;
}
