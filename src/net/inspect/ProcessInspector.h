#ifndef NET_INSPECT_PROCESSINSPECTOR_H
#define NET_INSPECT_PROCESSINSPECTOR_H

#include "net/inspect/Inspector.h"

#include <boost/noncopyable.hpp>

namespace net
{

class ProcessInspector : boost::noncopyable
{
public:
	void registerCommands(Inspector* ins);

	static std::string overview(HttpRequest::Method, const Inspector::ArgList&);
	static std::string pid(HttpRequest::Method, const Inspector::ArgList&);
	static std::string procStatus(HttpRequest::Method, const Inspector::ArgList&);
	static std::string openedFiles(HttpRequest::Method, const Inspector::ArgList&);
	static std::string threads(HttpRequest::Method, const Inspector::ArgList&);

	static std::string username_;
};

} // namespace net

#endif // NET_INSPECT_PROCESSINSPECTOR_H
