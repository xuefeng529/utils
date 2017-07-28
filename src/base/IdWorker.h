#ifndef BASE_IDWORKER_H
#define BASE_IDWORKER_H

#include "base/Mutex.h"

#include <boost/noncopyable.hpp>

#include <stdint.h>

namespace base
{

/// twitter snowflake�㷨
/// 64       63--------------22---------12---------0
/// ����λ   |     41λʱ��   |10λ������|12λ������|
class IdWorker : boost::noncopyable
{
public:
    IdWorker(uint32_t workerId)
        : lastTimestamp_(0),
          workerId_(workerId),
		  sequence_(0)
	{ }
	
	uint64_t nextId();

private:
    int64_t currentTimeMillis() const;
	int64_t nextMillis() const;

    base::MutexLock lock_;
	int64_t lastTimestamp_;
    const uint32_t workerId_;
	uint32_t sequence_;
};

} // namespace base

#endif // BASE_IDWORKER_H
