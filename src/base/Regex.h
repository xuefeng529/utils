#ifndef BASE_REGEX_H
#define BASE_REGEX_H

#include <boost/xpressive/xpressive_dynamic.hpp>
#include <string>

namespace base
{

namespace Regex
{

bool isMatch(const std::string& str, const std::string& re, bool ignoreCase = false)
{
	boost::xpressive::cregex reg = 
		(ignoreCase ? boost::xpressive::cregex::compile(re, boost::xpressive::icase)
		: boost::xpressive::cregex::compile(re));
	return boost::xpressive::regex_match(str, reg);
}

} // namespace Regex

} // namespace base

#endif // BASE_REGEX_H
