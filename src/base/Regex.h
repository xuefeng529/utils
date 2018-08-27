#ifndef BASE_REGEX_H
#define BASE_REGEX_H

#include <boost/xpressive/xpressive_dynamic.hpp>
#include <string>

namespace base
{

class Regex
{
public:
    static bool isMatch(const std::string& str, const std::string& re, bool ignoreCase = false)
    {
        boost::xpressive::sregex reg =
            (ignoreCase ? boost::xpressive::sregex::compile(re, boost::xpressive::icase)
            : boost::xpressive::sregex::compile(re));
        return boost::xpressive::regex_match(str, reg);
    }
};

} // namespace base

#endif // BASE_REGEX_H
