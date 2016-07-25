#include "net/inspect/PerformanceInspector.h"
#include "base/FileUtil.h"
#include "base/LogStream.h"
#include "base/ProcessInfo.h"

#ifdef HAVE_TCMALLOC
#include <gperftools/malloc_extension.h>
#include <gperftools/profiler.h>

namespace net
{

void PerformanceInspector::registerCommands(Inspector* ins)
{
	ins->add("pprof", "heap", PerformanceInspector::heap, "get heap information");
	ins->add("pprof", "growth", PerformanceInspector::growth, "get heap growth information");
	ins->add("pprof", "profile", PerformanceInspector::profile,
		"get cpu profiling information. CAUTION: blocking thread for 30 seconds!");
	ins->add("pprof", "cmdline", PerformanceInspector::cmdline, "get command line");
	ins->add("pprof", "memstats", PerformanceInspector::memstats, "get memory stats");
	ins->add("pprof", "memhistogram", PerformanceInspector::memhistogram, "get memory histogram");
	ins->add("pprof", "releasefreememory", PerformanceInspector::releaseFreeMemory, "release free memory");
}

std::string PerformanceInspector::heap(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string result;
	MallocExtension::instance()->GetHeapSample(&result);
	return std::string(result.data(), result.size());
}

std::string PerformanceInspector::growth(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string result;
	MallocExtension::instance()->GetHeapGrowthStacks(&result);
	return std::string(result.data(), result.size());
}

std::string PerformanceInspector::profile(HttpRequest::Method, const Inspector::ArgList&)
{
	std::string filename = "/tmp/" + ProcessInfo::procname();
	filename += ".";
	filename += ProcessInfo::pidString();
	filename += ".";
	filename += Timestamp::now().toString();
	filename += ".profile";

	std::string profile;
	if (ProfilerStart(filename.c_str()))
	{
		// FIXME: async
		CurrentThread::sleepUsec(30 * 1000 * 1000);
		ProfilerStop();
		FileUtil::readFile(filename, 1024*1024, &profile, NULL, NULL);
		::unlink(filename.c_str());
	}
	return profile;
}

std::string PerformanceInspector::cmdline(HttpRequest::Method, const Inspector::ArgList&)
{
	return "";
}

std::string PerformanceInspector::memstats(HttpRequest::Method, const Inspector::ArgList&)
{
	char buf[1024*64];
	MallocExtension::instance()->GetStats(buf, sizeof buf);
	return buf;
}

std::string PerformanceInspector::memhistogram(HttpRequest::Method, const Inspector::ArgList&)
{
	int blocks = 0;
	size_t total = 0;
	int histogram[kMallocHistogramSize] = { 0, };

	MallocExtension::instance()->MallocMemoryStats(&blocks, &total, histogram);
	LogStream s;
	s << "blocks " << blocks << "\ntotal " << total << "\n";
	for (int i = 0; i < kMallocHistogramSize; ++i)
		s << i << " " << histogram[i] << "\n";
	return s.buffer().toString();
}

std::string PerformanceInspector::releaseFreeMemory(HttpRequest::Method, const Inspector::ArgList&)
{
	char buf[256];
	snprintf(buf, sizeof buf, "memory release rate: %f\nAll free memory released.\n",
		MallocExtension::instance()->GetMemoryReleaseRate());
	MallocExtension::instance()->ReleaseFreeMemory();
	return buf;
}

} // namespace net

#endif
