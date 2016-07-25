#ifndef NET_INSPECT_PERFORMANCEINSPECTOR_H
#define NET_INSPECT_PERFORMANCEINSPECTOR_H

#include "net/inspect/Inspector.h"

#include <boost/noncopyable.hpp>

namespace net
{

class PerformanceInspector : boost::noncopyable
{
public:
	void registerCommands(Inspector* ins);

	static std::string heap(HttpRequest::Method, const Inspector::ArgList&);
	static std::string growth(HttpRequest::Method, const Inspector::ArgList&);
	static std::string profile(HttpRequest::Method, const Inspector::ArgList&);
	static std::string cmdline(HttpRequest::Method, const Inspector::ArgList&);
	static std::string memstats(HttpRequest::Method, const Inspector::ArgList&);
	static std::string memhistogram(HttpRequest::Method, const Inspector::ArgList&);
	static std::string releaseFreeMemory(HttpRequest::Method, const Inspector::ArgList&);

	static std::string symbol(HttpRequest::Method, const Inspector::ArgList&);
};

} // namespace net

#endif  // NET_INSPECT_PERFORMANCEINSPECTOR_H
