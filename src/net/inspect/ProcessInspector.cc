#include "net/inspect/ProcessInspector.h"
#include "base/Slice.h"
#include "base/FileUtil.h"
#include "base/ProcessInfo.h"

#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>


namespace inspect
{

std::string uptime(base::Timestamp now, base::Timestamp start, bool showMicroseconds)
{
	char buf[256];
	int64_t age = now.microSecondsSinceEpoch() - start.microSecondsSinceEpoch();
	int seconds = static_cast<int>(age / base::Timestamp::kMicroSecondsPerSecond);
	int days = seconds / 86400;
	int hours = (seconds % 86400) / 3600;
	int minutes = (seconds % 3600) / 60;
	if (showMicroseconds)
	{
		int microseconds = static_cast<int>(age % base::Timestamp::kMicroSecondsPerSecond);
		snprintf(buf, sizeof buf, "%d days %02d:%02d:%02d.%06d",
			days, hours, minutes, seconds % 60, microseconds);
	}
	else
	{
		snprintf(buf, sizeof buf, "%d days %02d:%02d:%02d",
			days, hours, minutes, seconds % 60);
	}
	return buf;
}

long getLong(const std::string& procStatus, const char* key)
{
	long result = 0;
	size_t pos = procStatus.find(key);
	if (pos != std::string::npos)
	{
		result = ::atol(procStatus.c_str() + pos + strlen(key));
	}
	return result;
}

std::string getProcessName(const std::string& procStatus)
{
	std::string result;
	size_t pos = procStatus.find("Name:");
	if (pos != std::string::npos)
	{
		pos += strlen("Name:");
		while (procStatus[pos] == '\t')
			++pos;
		size_t eol = pos;
		while (procStatus[eol] != '\n')
			++eol;
		result = procStatus.substr(pos, eol - pos);
	}
	return result;
}

base::Slice next(base::Slice data)
{
	const char* sp = static_cast<const char*>(::memchr(data.data(), ' ', data.size()));
	if (sp)
	{
		data.remove_prefix(static_cast<int>(sp + 1 - data.data()));
		return data;
	}
	return "";
}

base::ProcessInfo::CpuTime getCpuTime(base::Slice data)
{
	base::ProcessInfo::CpuTime t;

	for (int i = 0; i < 10; ++i)
	{
		data = next(data);
	}
	long utime = strtol(data.data(), NULL, 10);
	data = next(data);
	long stime = strtol(data.data(), NULL, 10);
	const double hz = static_cast<double>(base::ProcessInfo::clockTicksPerSecond());
	t.userSeconds = static_cast<double>(utime) / hz;
	t.systemSeconds = static_cast<double>(stime) / hz;
	return t;
}

int stringPrintf(std::string* out, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

int stringPrintf(std::string* out, const char* fmt, ...)
{
	char buf[256];
	va_list args;
	va_start(args, fmt);
	int ret = vsnprintf(buf, sizeof buf, fmt, args);
	va_end(args);
	out->append(buf);
	return ret;
}

} // namespace inspect

namespace net
{

std::string ProcessInspector::username_ = base::ProcessInfo::username();

void ProcessInspector::registerCommands(Inspector* ins)
{
	ins->add("proc", "overview", ProcessInspector::overview, "print basic overview");
	ins->add("proc", "pid", ProcessInspector::pid, "print pid");
	ins->add("proc", "status", ProcessInspector::procStatus, "print /proc/self/status");
	ins->add("proc", "threads", ProcessInspector::threads, "list /proc/self/task");
}

std::string ProcessInspector::overview(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string result;
	result.reserve(1024);
	base::Timestamp now = base::Timestamp::now();
	result += "Page generated at ";
	result += now.toFormattedString();
	result += " (UTC)\nStarted at ";
	result += base::ProcessInfo::startTime().toFormattedString();
	result += " (UTC), up for ";
	result += inspect::uptime(now, base::ProcessInfo::startTime(), true);
	result += "\n";

	std::string procStatus = base::ProcessInfo::procStatus();
	result += inspect::getProcessName(procStatus);
	result += " (";
	result += base::ProcessInfo::exePath();
	result += ") running as ";
	result += username_;
	result += " on ";
	result += base::ProcessInfo::hostname();
	result += "\n";

	if (base::ProcessInfo::isDebugBuild())
	{
		result += "WARNING: debug build!\n";
	}

	inspect::stringPrintf(&result, "pid %d, num of threads %ld, bits %zd\n",
		base::ProcessInfo::pid(), inspect::getLong(procStatus, "Threads:"), CHAR_BIT * sizeof(void*));

	result += "Virtual memory: ";
	inspect::stringPrintf(&result, "%.3f MiB, ",
		static_cast<double>(inspect::getLong(procStatus, "VmSize:")) / 1024.0);

	result += "RSS memory: ";
	inspect::stringPrintf(&result, "%.3f MiB\n",
		static_cast<double>(inspect::getLong(procStatus, "VmRSS:")) / 1024.0);

	inspect::stringPrintf(&result, "Opened files: %d, limit: %d\n",
		base::ProcessInfo::openedFiles(), base::ProcessInfo::maxOpenFiles());

	base::ProcessInfo::CpuTime t = base::ProcessInfo::cpuTime();
	inspect::stringPrintf(&result, "User time: %12.3fs\nSys time:  %12.3fs\n",
		t.userSeconds, t.systemSeconds);

	return result;
}

std::string ProcessInspector::pid(HttpRequest::Method, const Inspector::ArgList&)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%d", base::ProcessInfo::pid());
	return buf;
}

std::string ProcessInspector::procStatus(HttpRequest::Method, const Inspector::ArgList&)
{
	return base::ProcessInfo::procStatus();
}

std::string ProcessInspector::openedFiles(HttpRequest::Method, const Inspector::ArgList&)
{
	char buf[32];
	snprintf(buf, sizeof buf, "%d", base::ProcessInfo::openedFiles());
	return buf;
}

std::string ProcessInspector::threads(HttpRequest::Method, const Inspector::ArgList&)
{
	std::vector<pid_t> threads = base::ProcessInfo::threads();
	std::string result = "  TID NAME             S    User Time  System Time\n";
	result.reserve(threads.size() * 64);
	std::string stat;
	for (size_t i = 0; i < threads.size(); ++i)
	{
		char buf[256];
		int tid = threads[i];
		snprintf(buf, sizeof buf, "/proc/%d/task/%d/stat", base::ProcessInfo::pid(), tid);
		if (base::FileUtil::readFile(buf, 65536, &stat) == 0)
		{
			base::Slice name = base::ProcessInfo::procname(stat);
			const char* rp = name.data() + name.size();
			assert(*rp == ')');
			const char* state = rp + 2;
			*const_cast<char*>(rp) = '\0';  // don't do this at home
			base::Slice data(stat);
			data.remove_prefix(static_cast<int>(state - data.data() + 2));
			base::ProcessInfo::CpuTime t = inspect::getCpuTime(data);
			snprintf(buf, sizeof buf, "%5d %-16s %c %12.3f %12.3f\n",
				tid, name.data(), *state, t.userSeconds, t.systemSeconds);
			result += buf;
		}
	}
	return result;
}

} // namespace net
