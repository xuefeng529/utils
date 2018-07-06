#include "plugins/etcd/LeaderSelector.h"
#include "base/Logging.h"

#include <iostream>

#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

char g_nodeName[1024];

void takeLeader()
{
    LOG_INFO << "I am leader - " << g_nodeName;
}

int main(int argc, char* argv[])
{
    base::Logger::setLogLevel(base::Logger::INFO);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(static_cast<unsigned>(time(NULL)));
    snprintf(g_nodeName, sizeof(g_nodeName), "%ld-%ld-%d-%d", tv.tv_sec, tv.tv_usec, getpid(), rand());

    LOG_INFO << "start ...";
    plugins::etcd::LeaderSelector leaderSelector("127.0.0.1:2379", 3, "leader_follower", g_nodeName, takeLeader);
    leaderSelector.start();
    std::string line;
    std::getline(std::cin, line);
    return 0;
}