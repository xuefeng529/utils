#ifndef BASE_UNIQUEIDGENERATOR_H
#define BASE_UNIQUEIDGENERATOR_H

#include <boost/noncopyable.hpp>

#include <stdint.h>

namespace base
{

/// twitter snowflake算法
/// 64       63--------------22---------12---------0
/// 符号位   |     41位时间   |10位机器码|12位自增码|
class UniqueIdGenerator : boost::noncopyable
{
public:
	UniqueIdGenerator()
		: lastStamp_(0),
		  machineId_(0),
		  sequence_(0)
	{ }
	
    void setMachineId(int32_t machineId)
	{ machineId_ = machineId; }

	uint64_t getUniqueId();

private:
	uint64_t waitNextMillisecond() const;

	mutable uint64_t lastStamp_;
    uint32_t machineId_;
	uint32_t sequence_;
};

} // namespace base

#endif // BASE_UNIQUEIDGENERATOR_H
