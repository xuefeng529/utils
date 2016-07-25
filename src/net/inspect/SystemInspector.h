#ifndef NET_INSPECT_SYSTEMINSPECTOR_H
#define NET_INSPECT_SYSTEMINSPECTOR_H

#include "net/inspect/Inspector.h"

namespace net
{

class SystemInspector : boost::noncopyable
{
public:
	void registerCommands(Inspector* ins);

	static std::string overview(HttpRequest::Method, const Inspector::ArgList&);
	static std::string loadavg(HttpRequest::Method, const Inspector::ArgList&);
	static std::string version(HttpRequest::Method, const Inspector::ArgList&);
	static std::string cpuinfo(HttpRequest::Method, const Inspector::ArgList&);
	static std::string meminfo(HttpRequest::Method, const Inspector::ArgList&);
	static std::string stat(HttpRequest::Method, const Inspector::ArgList&);
};

} // namespace net

#endif // NET_INSPECT_SYSTEMINSPECTOR_H
