#include "plugins/zk/LeaderSelector.h"
#include "base/Logging.h"

#include <iostream>

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

char g_nodeName[1024];

void takeLeader()
{
    std::cout << "I am leader - " << g_nodeName << std::endl;
}

int main(int argc, char* argv[])
{
    base::Logger::setLogLevel(base::Logger::INFO);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(static_cast<unsigned>(time(NULL)));
    snprintf(g_nodeName, sizeof(g_nodeName), "%ld-%ld-%d-%d", tv.tv_sec, tv.tv_usec, getpid(), rand());

    std::cout << "start ..." << std::endl;
    plugins::zk::LeaderSelector leaderSelector(argv[1], atoi(argv[2]), "/leader_follower_test", g_nodeName, takeLeader);
	leaderSelector.start(); 
    std::string line;
    std::getline(std::cin, line);
    return 0;
}
