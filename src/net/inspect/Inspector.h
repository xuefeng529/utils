#ifndef NET_INSPECT_INSPECTOR_H
#define NET_INSPECT_INSPECTOR_H

#include "base/Mutex.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpServer.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <map>
#include <vector>

namespace net
{

class ProcessInspector;
class PerformanceInspector;
class SystemInspector;

// An internal inspector of the running process, usually a singleton.
// Better to run in a separated thread, as some method may block for seconds
class Inspector : boost::noncopyable
{
public:
	typedef std::vector<std::string> ArgList;
	typedef boost::function<std::string(HttpRequest::Method, const ArgList& args)> Callback;
	Inspector(EventLoop* loop,
			  const InetAddress& httpAddr,
			  const std::string& name);
	~Inspector();

	void add(const std::string& module,
			 const std::string& command,
			 const Callback& cb,
			 const std::string& help);
	void remove(const std::string& module, const std::string& command);

private:
	typedef std::map<std::string, Callback> CommandList;
	typedef std::map<std::string, std::string> HelpList;

	void start();
	void onRequest(const net::TcpConnectionPtr& conn, const net::HttpRequest& req);

	HttpServer server_;
	boost::scoped_ptr<ProcessInspector> processInspector_;
	boost::scoped_ptr<PerformanceInspector> performanceInspector_;
	boost::scoped_ptr<SystemInspector> systemInspector_;
	base::MutexLock mutex_;
	std::map<std::string, CommandList> modules_;
	std::map<std::string, HelpList> helps_;
};

} // namespace net

#endif  // NET_INSPECT_INSPECTOR_H
