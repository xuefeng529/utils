#include "net/inspect/SystemInspector.h"
#include "base/FileUtil.h"
#include "base/Timestamp.h"

#include <sys/utsname.h>

namespace inspect
{

std::string uptime(base::Timestamp now, base::Timestamp start, bool showMicroseconds);
long getLong(const std::string& content, const char* key);
int stringPrintf(std::string* out, const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));

} // namespace inspect

namespace net
{

void SystemInspector::registerCommands(Inspector* ins)
{
	ins->add("sys", "overview", SystemInspector::overview, "print system overview");
	ins->add("sys", "loadavg", SystemInspector::loadavg, "print /proc/loadavg");
	ins->add("sys", "version", SystemInspector::version, "print /proc/version");
	ins->add("sys", "cpuinfo", SystemInspector::cpuinfo, "print /proc/cpuinfo");
	ins->add("sys", "meminfo", SystemInspector::meminfo, "print /proc/meminfo");
	ins->add("sys", "stat", SystemInspector::stat, "print /proc/stat");
}

std::string SystemInspector::loadavg(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string loadavg;
	base::FileUtil::readFile("/proc/loadavg", 65536, &loadavg);
	return loadavg;
}

std::string SystemInspector::version(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string version;
	base::FileUtil::readFile("/proc/version", 65536, &version);
	return version;
}

std::string SystemInspector::cpuinfo(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string cpuinfo;
	base::FileUtil::readFile("/proc/cpuinfo", 65536, &cpuinfo);
	return cpuinfo;
}

std::string SystemInspector::meminfo(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string meminfo;
	base::FileUtil::readFile("/proc/meminfo", 65536, &meminfo);
	return meminfo;
}

std::string SystemInspector::stat(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string stat;
	base::FileUtil::readFile("/proc/stat", 65536, &stat);
	return stat;
}

std::string SystemInspector::overview(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string result;
	result.reserve(1024);
	base::Timestamp now = base::Timestamp::now();
	result += "Page generated at ";
	result += now.toFormattedString();
	result += " (UTC)\n";
	// Hardware and OS
	{
		struct utsname un;
		if (::uname(&un) == 0)
		{
			inspect::stringPrintf(&result, "Hostname: %s\n", un.nodename);
			inspect::stringPrintf(&result, "Machine: %s\n", un.machine);
			inspect::stringPrintf(&result, "OS: %s %s %s\n", un.sysname, un.release, un.version);
		}
	}
	std::string stat;
	base::FileUtil::readFile("/proc/stat", 65536, &stat);
	base::Timestamp bootTime(base::Timestamp::kMicroSecondsPerSecond * inspect::getLong(stat, "btime "));
	result += "Boot time: ";
	result += bootTime.toFormattedString(false /* show microseconds */);
	result += " (UTC)\n";
	result += "Up time: ";
	result += inspect::uptime(now, bootTime, false /* show microseconds */);
	result += "\n";

	// CPU load
	{
		std::string loadavg;
		base::FileUtil::readFile("/proc/loadavg", 65536, &loadavg);
		inspect::stringPrintf(&result, "Processes created: %ld\n", inspect::getLong(stat, "processes "));
		inspect::stringPrintf(&result, "Loadavg: %s\n", loadavg.c_str());
	}

	// Memory
	{
	  std::string meminfo;
	  base::FileUtil::readFile("/proc/meminfo", 65536, &meminfo);
	  long total_kb = inspect::getLong(meminfo, "MemTotal:");
	  long free_kb = inspect::getLong(meminfo, "MemFree:");
	  long buffers_kb = inspect::getLong(meminfo, "Buffers:");
	  long cached_kb = inspect::getLong(meminfo, "Cached:");

	  inspect::stringPrintf(&result, "Total Memory: %6ld MiB\n", total_kb / 1024);
	  inspect::stringPrintf(&result, "Free Memory:  %6ld MiB\n", free_kb / 1024);
	  inspect::stringPrintf(&result, "Buffers:      %6ld MiB\n", buffers_kb / 1024);
	  inspect::stringPrintf(&result, "Cached:       %6ld MiB\n", cached_kb / 1024);
	  inspect::stringPrintf(&result, "Real Used:    %6ld MiB\n", (total_kb - free_kb - buffers_kb - cached_kb) / 1024);
	  inspect::stringPrintf(&result, "Real Free:    %6ld MiB\n", (free_kb + buffers_kb + cached_kb) / 1024);
	}
	
	return result;
}

} // namespace net
