#include "base/LockFree.h"
#include "base/Thread.h"
#include "base/CurrentThread.h"
#include "base/Logging.h"

#include <boost/bind.hpp>
#include <iostream>

#include <sched.h>

const int64_t kProduceSize = 1000000;

base::lockfree::ArrayLockFreeQueue<int64_t, 10000>  theQueue;

void produceThread()
{
    LOG_INFO << "produce begin: tid = " << base::CurrentThread::tid();
    int64_t n = 0;
    while (n < kProduceSize)
    {
        if (theQueue.push(n))
        {
            LOG_INFO << "push " << n;
            n++;
        }
        else
        {
            sched_yield();
        }
    }
    LOG_INFO << "produce done: tid = " << base::CurrentThread::tid();
}

void consumeThread()
{
    LOG_INFO << "consume begin: tid = " << base::CurrentThread::tid();
    int64_t n = 0;
    int64_t v;
    while (n < kProduceSize * 2)
    {
        if (theQueue.pop(v))
        {
            LOG_INFO << "pop " << v;
            n++;
        }  
        else
        {
            sched_yield();
        }
    }
    
    LOG_INFO << "consume done: tid = " << base::CurrentThread::tid();
}

int main(int argc, char* argv[])
{
    base::Thread producerThread1(boost::bind(produceThread));
    base::Thread producerThread2(boost::bind(produceThread));
    base::Thread consumerThread(boost::bind(consumeThread));
    producerThread1.start();
    producerThread2.start();
    consumerThread.start();
    producerThread1.join();
    producerThread2.join();
    consumerThread.join();
    return 0;
}