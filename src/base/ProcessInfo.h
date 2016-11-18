#ifndef BASE_PROCESSINFO_H
#define BASE_PROCESSINFO_H

#include "base/Timestamp.h"
#include "Slice.h"

#include <string>
#include <vector>

namespace base
{
namespace ProcessInfo
{

pid_t pid();
std::string pidString();
uid_t uid();
std::string username();
uid_t euid();
Timestamp startTime();
int clockTicksPerSecond();
int pageSize();
bool isDebugBuild();  // constexpr

std::string hostname();
std::string procname();
Slice procname(const std::string& stat);

/// read /proc/self/status
std::string procStatus();

/// read /proc/self/stat
std::string procStat();

/// read /proc/self/task/tid/stat
std::string threadStat();

/// readlink /proc/self/exe
std::string exePath();

int openedFiles();
int maxOpenFiles();

struct CpuTime
{
	double userSeconds;
	double systemSeconds;

	CpuTime() : userSeconds(0.0), systemSeconds(0.0) { }
};
CpuTime cpuTime();

int numThreads();
std::vector<pid_t> threads();
/// 绑定线程到指定编号的cpu
void bindThreadToCpu(int cpuIndex);
/// 获取cpu核心数
size_t getCpuCoresCount();

} // namespace ProcessInfo
} // namespace base

#endif  // BASE_PROCESSINFO_H
