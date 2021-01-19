#include "base/BloomFilter.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

namespace base
{

const size_t BloomFilter::kDefaultSize;

namespace
{

size_t hash(int seed, const std::string& value)
{
	size_t result = 0;
	size_t len = value.length();
	for (size_t i = 0; i < len; i++)
	{
		result = seed * result + value[i];
	}

	return result;
}

typedef boost::function<size_t(const std::string& value)> HashFunction;
std::vector<HashFunction> g_functions;

class Initializer
{
public:
	Initializer()
	{
		/// 定义一个8个元素的质数数组
		std::vector<int> seeds = {3, 5, 7, 11, 13, 31, 37, 61};
		for (size_t i = 0; i < seeds.size(); ++i)
		{
			g_functions.push_back(boost::bind(hash, seeds[i], _1));
		}
	}
};

Initializer g_initializer;

}

BloomFilter::BloomFilter()
	: bt_(new std::bitset<kDefaultSize>())
{
}

void BloomFilter::add(const std::string& value)
{
	assert(!value.empty());
	for (size_t i = 0; i < g_functions.size(); i++)
	{
		size_t index = g_functions[i](value) % kDefaultSize;
		bt_->set(index);
	}
}

bool BloomFilter::mightContain(const std::string& value)
{
	assert(!value.empty());
	for (size_t i = 0; i < g_functions.size(); i++)
	{
		size_t index = g_functions[i](value) % kDefaultSize;
		if (!bt_->test(index))
		{
			return false;
		}
	}

	return true;
}

} // namespace base
