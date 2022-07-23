#include "base/MemoryPool.h"
#include "base/Mutex.h"
#include "base/Singleton.h"
#include "base/SpinLock.h"
#include "base/Timestamp.h"

#include <boost/scoped_ptr.hpp>

#include <array>
#include <iostream>
#include <string.h>

struct Block
{
	int i_;
	float f_;

	Block()
	{
		//std::cout << "Block" << std::endl;
	}

	~Block() 
	{
		//std::cout << "~Block" << std::endl;
	}

	void* operator new(size_t n)
	{
		//return base::MTObjectPool<Block, base::MutexLock>::malloc();
		std::cout << "operator new, size: " << n << std::endl;
		return base::Singleton<base::MemoryPool<sizeof(Block)> >::instance().malloc();
	}

	void operator delete(void* ptr, size_t n)
	{
		//base::MTObjectPool<Block, base::MutexLock>::free(reinterpret_cast<Block*>(ptr));
		std::cout << "operator delete, size: " << n << std::endl;
		base::Singleton<base::MemoryPool<sizeof(Block)> >::instance().free(ptr);
	}
};

template<class T, size_t N>
class STArray
{
public:
	STArray()
	{
		elements_ = reinterpret_cast<T*>(base::Singleton<base::MemoryPool<sizeof(T)*N> >::instance().malloc());
	}

	~STArray()
	{
		base::Singleton<base::MemoryPool<sizeof(T)*N> >::instance().free(reinterpret_cast<void*>(elements_));
	}

	T& operator[](size_t i)
	{
		return elements_[i];
	}

	size_t size() const { return N; }

private:
	T* elements_;
};

template<class Lock, class Element, size_t N>
class MTArray
{
public:
	MTArray()
	{
		elements_ = reinterpret_cast<Element*>(
			base::Singleton<base::MTMemoryPool<Lock, sizeof(Element)*N> >::instance().malloc());
	}

	~MTArray()
	{
		base::Singleton<base::MTMemoryPool<Lock, sizeof(Element)*N> >::instance().free(reinterpret_cast<void*>(elements_));
	}

	Element& operator[](size_t i)
	{
		return elements_[i];
	}

	size_t size() const { return N; }

private:
	Element* elements_;
};

void testMemory()
{
	std::cout << "test memory" << std::endl;
	Block* blocks[1000];
	std::cout << "Block size: " << sizeof(Block) << std::endl;
	int64_t begin = base::Timestamp::now().microSecondsSinceEpoch();
	for (int n = 0; n < 1; n++)
	{
		for (int i = 0; i < 1; i++)
		{
			blocks[i] = new Block;
		}

		for (int i = 0; i < 1; i++)
		{
			delete blocks[i];
		}
	}

	int64_t end = base::Timestamp::now().microSecondsSinceEpoch();
	std::cout << "take time: " << end - begin << "[us]" << std::endl;
}

void testArray()
{
	std::cout << "---test std array---" << std::endl;
	int64_t begin = base::Timestamp::now().microSecondsSinceEpoch();
	for (int n = 0; n < 1000; n++)
	{
		std::array<Block, 100>* array[1000];
		for (int i = 0; i < 1000; i++)
		{
			array[i] = new std::array<Block, 100>();
		}

		for (int i = 0; i < 1000; i++)
		{
			delete array[i];
		}
	}

	int64_t end = base::Timestamp::now().microSecondsSinceEpoch();
	std::cout << "take time: " << end - begin << "[us]" << std::endl;
}

void testSTArray()
{
	std::cout << "---test single thread array---" << std::endl;
	int64_t begin = base::Timestamp::now().microSecondsSinceEpoch();
	for (int n = 0; n < 1000; n++)
	{
		STArray<Block, 100>* array[1000];
		for (int i = 0; i < 1000; i++)
		{
			array[i] = new STArray<Block, 100>();
		}

		for (int i = 0; i < 1000; i++)
		{
			delete array[i];
		}
	}
	
	int64_t end = base::Timestamp::now().microSecondsSinceEpoch();
	std::cout << "take time: " << end - begin << "[us]" << std::endl;

	STArray<Block, 10> array;
	std::cout << "array size: " << array.size() << std::endl;
	for (size_t i = 0; i < array.size(); i++)
	{
		array[i].i_ = i;
		array[i].f_ = i * 0.1f;
	}

	for (size_t i = 0; i < array.size(); i++)
	{
		std::cout << array[i].i_ << " " << array[i].f_ << std::endl;
	}
}

void testMTArray()
{
	std::cout << "---test multi thread array---" << std::endl;
	int64_t begin = base::Timestamp::now().microSecondsSinceEpoch();
	for (int n = 0; n < 1000; n++)
	{
		MTArray<base::MutexLock, Block, 100>* array[1000];
		for (int i = 0; i < 1000; i++)
		{
			array[i] = new MTArray<base::MutexLock, Block, 100>();
		}

		for (int i = 0; i < 1000; i++)
		{
			delete array[i];
		}
	}

	int64_t end = base::Timestamp::now().microSecondsSinceEpoch();
	std::cout << "take time: " << end - begin << "[us]" << std::endl;

	MTArray<base::MutexLock, Block, 10> array;
	std::cout << "array size: " << array.size() << std::endl;
	for (size_t i = 0; i < array.size(); i++)
	{
		array[i].i_ = i;
		array[i].f_ = i * 0.1f;
	}

	for (size_t i = 0; i < array.size(); i++)
	{
		std::cout << array[i].i_ << " " << array[i].f_ << std::endl;
	}
}

int main(int argc, char* argv[])
{
	testArray();
	testSTArray();
	testMTArray();
	testMemory();
	std::string line;
	std::getline(std::cin, line);
	return 0;
}
