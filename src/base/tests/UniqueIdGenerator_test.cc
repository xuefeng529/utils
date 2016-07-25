#include "base/UniqueIdGenerator.h"
#include "base/Thread.h"
#include "base/Mutex.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <stdint.h>
#include <set>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

std::set<uint64_t> g_id;
base::MutexLock g_mutex;
base::UniqueIdGenerator idGenerator;

void buildId()
{
	for (size_t i = 0; i < 100000000; i++)
	{
		base::MutexLockGuard lock(g_mutex);
		uint64_t unquie = idGenerator.getUniqueId();
		//fprintf(stdout, "Thread id %d[%" PRId64 "]\n", base::CurrentThread::tid(), unquie);
		std::set<uint64_t>::const_iterator it = g_id.find(unquie);
		if (it != g_id.end())
		{
			fprintf(stderr, "the same id: %" PRId64 "\n", unquie);
		}
		else
		{
			g_id.insert(unquie);
		}

		//fprintf(stdout, "id [%" PRId64 "]\n", unquie);
	}
}

int main(int argc, char* argv[])
{
	fprintf(stdout, "begin to generate unique id\n");
	idGenerator.setMachineId(1);
	base::Thread thread1(boost::bind(buildId));
	base::Thread thread2(boost::bind(buildId));
	base::Thread thread3(boost::bind(buildId));
	base::Thread thread4(boost::bind(buildId));
	thread1.start();
	thread2.start();
	thread3.start();
	thread4.start();
	thread1.join();
	fprintf(stdout, "thread1 done.\n");
	thread2.join();
	fprintf(stdout, "thread2 done.\n");
	thread3.join();
	fprintf(stdout, "thread3 done.\n");
	thread4.join();
	fprintf(stdout, "thread4 done.\n");
	return 0;
}