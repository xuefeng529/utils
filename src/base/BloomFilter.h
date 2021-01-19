#ifndef BASE_BLOOMFILTER_H
#define BASE_BLOOMFILTER_H

#include <bitset>
#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace base
{

class BloomFilter : boost::noncopyable
{
public:
	BloomFilter();
	void add(const std::string& value);
	bool mightContain(const std::string& value);
 
private:
	static const size_t kDefaultSize = 2 << 29;
	boost::scoped_ptr<std::bitset<kDefaultSize> > bt_;
};

} // namespace base

#endif // BASE_BLOOMFILTER_H
