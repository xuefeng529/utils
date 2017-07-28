#include "base/IdWorker.h"
#include "base/Thread.h"

#include <boost/bind.hpp>
#include <set>

#include <stdio.h>
#include <stdint.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

std::set<uint64_t> g_ids;
base::MutexLock g_lock;

void genThread(base::IdWorker& idWorker)
{
	for (size_t i = 0; i < 10000000; i++)
	{
        uint64_t id = idWorker.nextId();
        std::pair<std::set<uint64_t>::iterator, bool> ret;

        {
            base::MutexLockGuard lock(g_lock);
            ret = g_ids.insert(id);
        }
         
        if (!ret.second)
        {
            fprintf(stderr, "the same id: %" PRId64 "\n", id);
        }       
	}
}

int main(int argc, char* argv[])
{
	fprintf(stdout, "begin to generate unique id\n");
    base::IdWorker idWorker(1);	
    base::Thread thread1(boost::bind(genThread, boost::ref(idWorker)));
    base::Thread thread2(boost::bind(genThread, boost::ref(idWorker)));
    base::Thread thread3(boost::bind(genThread, boost::ref(idWorker)));  
	thread1.start();
	thread2.start();
	thread3.start();	
	thread1.join();
	fprintf(stdout, "thread1 done.\n");
	thread2.join();
	fprintf(stdout, "thread2 done.\n");
	thread3.join();
	fprintf(stdout, "thread3 done.\n");
	return 0;
}