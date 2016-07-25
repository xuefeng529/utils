#include "base/UniqueIdGenerator.h"
#include "base/Logging.h"

#include <sys/time.h>
#include <strings.h>
#include <unistd.h>

namespace base
{
  
namespace
{

int64_t getCurrentTime()
{
	struct timeval nowTime;
	gettimeofday(&nowTime, NULL);
	int64_t ms = nowTime.tv_sec * 1000 + nowTime.tv_usec / 1000;
	return ms;
}

} // namespace


uint64_t UniqueIdGenerator::waitNextMillisecond() const
{
	uint64_t cur = 0;
	do
	{
		cur = getCurrentTime();
	} while (cur <= lastStamp_);
	return cur;
}

uint64_t UniqueIdGenerator::getUniqueId()
{
	uint64_t uniqueId = 0;
	uint32_t incrementId = 0;
	uint64_t nowTime = getCurrentTime();
	if (nowTime < lastStamp_)
	{
		LOG_FATAL << "nowTime < lastStamp";
	}
	else if (nowTime == lastStamp_)
	{
		sequence_++;
		if (sequence_ == 0x1000)
		{
			nowTime = waitNextMillisecond();
			sequence_ = 0;
		}
		incrementId = sequence_ & 0xFFF;
	}
	lastStamp_ = nowTime;
	uniqueId = nowTime << 22;
	uniqueId |= (machineId_ & 0x3ff) << 12;
	uniqueId |= incrementId;
	return uniqueId;
}

} // namespace base
