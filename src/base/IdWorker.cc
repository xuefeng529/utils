#include "base/IdWorker.h"
#include "base/Logging.h"

#include <sys/time.h>
#include <strings.h>
#include <unistd.h>

namespace base
{
  
int64_t IdWorker::currentTimeMillis() const
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int64_t ms = now.tv_sec * 1000 + now.tv_usec / 1000;
    return ms;
}

int64_t IdWorker::nextMillis() const
{
    int64_t now = currentTimeMillis();
    while (now <= lastTimestamp_)
    {
        now = currentTimeMillis();
    }
    return now;
}

uint64_t IdWorker::nextId()
{
    base::MutexLockGuard lock(lock_);   
    int64_t timestamp = currentTimeMillis();
    if (timestamp < lastTimestamp_)
	{
        LOG_FATAL << "Clock moved backwards.Refusing to generate id for "
            << lastTimestamp_ - timestamp << " milliseconds";
	}

    if (timestamp == lastTimestamp_)
	{
		sequence_++;
		if (sequence_ == 0x1000)
		{
            timestamp = nextMillis();
			sequence_ = 0;
		}
        sequence_ &= 0xFFF;
	}
    lastTimestamp_ = timestamp;
    uint64_t nextId = static_cast<uint64_t>(timestamp) << 22
        | (workerId_ & 0x3ff) << 12 | sequence_;
    return nextId;
}

} // namespace base
