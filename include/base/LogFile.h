#ifndef BASE_LOGFILE_H
#define BASE_LOGFILE_H

#include "base/Mutex.h"
#include "base/TimeZone.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>

#include <stdint.h>

namespace base
{

namespace FileUtil
{
class AppendFile;
}

class LogFile : boost::noncopyable
{
 public:
  LogFile(const std::string& dir,
		  const std::string& basename,
          size_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024);
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
  void append_unlocked(const char* logline, int len);

  static std::string getLogFileName(const std::string& dir, const std::string& basename, time_t* now);

  const std::string dir_;
  const std::string basename_;
  const size_t rollSize_;
  const int flushInterval_;
  const int checkEveryN_;

  int count_;

  boost::scoped_ptr<MutexLock> mutex_;
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
  boost::scoped_ptr<FileUtil::AppendFile> file_;

  const static int kRollPerSeconds_ = 60*60*24;
};

extern TimeZone g_logTimeZone;

}

#endif  // BASE_LOGFILE_H
