#include "plugins/zk/Client.h"
#include "base/Logging.h"
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <stdio.h>
#include <iostream>

using namespace plugins::zk;

void testCreate(ErrorCode errcode, const std::string& path)
{
    if (errcode == kOk) 
    {
        printf("testCreate-[kOk] path=%s\n", path.c_str());
    }
    else if (errcode == kError) 
    {
        printf("testCreate-[kError] path=%s\n", path.c_str());
    }
    else if (errcode == kNotExist) 
    {
        printf("testCreate-[kNotExist] path=%s\n", path.c_str());
    }
    else if (errcode == kExisted) 
    {
        printf("testCreate-[kExisted] path=%s\n", path.c_str());
    }
}

void testExist(ErrorCode errcode, const std::string& path, const NodeStat* stat) 
{
    if (errcode == kOk) 
    {
        printf("testExist-[kOk] path=%s version=%d\n", path.c_str(), stat->version);
    }
    else if (errcode == kDeleted)
    {
        printf("testExist-[kDeleted] path=%s\n", path.c_str());
    }
    else if (errcode == kError) 
    {
        printf("testExist-[kError] path=%s\n", path.c_str());
    }
    else if (errcode == kNotExist)
    {
        printf("testExist-[kNotExist] path=%s\n", path.c_str());
    }
}

void testGet(ErrorCode errcode, const std::string& path, const std::string& data) 
{
    if (errcode == kOk) 
    {
        printf("testGet-[kOk] path=%s value=%s\n", path.c_str(), data.c_str());
    }
    else if (errcode == kDeleted) 
    {
        printf("testGet-[kDeleted] path=%s\n", path.c_str());
    }
    else if (errcode == kError) 
    {
        printf("testGet-[kError] path=%s\n", path.c_str());
    }
    else if (errcode == kNotExist)
    {
        printf("testGet-[kNotExist] path=%s\n", path.c_str());
    }
}

void testGetChildren(ErrorCode errcode, const std::string& path, const std::vector<std::string> children) 
{
    if (errcode == kOk) 
    {
        printf("testGetChildren-[kOk] path=%s count=%" PRIu64"\n", path.c_str(), children.size());
        for (size_t i = 0; i < children.size(); i++)
        {
            printf("child: %s\n", children[i].c_str());
        }
    }
    else if (errcode == kDeleted)
    {
        printf("testGetChildren-[kDeleted] path=%s\n", path.c_str());
    }
    else if (errcode == kError) 
    {
        printf("testGetChildren-[kError] path=%s\n", path.c_str());
    }
    else if (errcode == kNotExist) 
    {
        printf("testGetChildren-[kNotExist] path=%s\n", path.c_str());
    }
}

void testSet(ErrorCode errcode, const std::string& path, const NodeStat* stat) 
{
    if (errcode == kOk) 
    {
        printf("testSet-[kOk] path=%s version=%d\n", path.c_str(), stat->version);
    }
    else if (errcode == kError) 
    {
        printf("testSet-[kError] path=%s\n", path.c_str());
    }
    else if (errcode == kNotExist) 
    {
        printf("testSet-[kNotExist] path=%s\n", path.c_str());
    }
}

void testDelete(ErrorCode errcode, const std::string& path) 
{
    if (errcode == kOk) 
    {
        printf("testDelete-[kOk] path=%s\n", path.c_str());
    }
    else if (errcode == kError) 
    {
        printf("testDelete-[kError] path=%s\n", path.c_str());
    }
    else if (errcode == kNotExist) 
    {
        printf("testDelete-[kNotExist] path=%s\n", path.c_str());
    }
    else if (errcode == kNotEmpty) 
    {
        printf("testDelete-[kNotEmpty] path=%s\n", path.c_str());
    }
}

int main(int argc, char* argv[])
{
    base::Logger::setLogLevel(base::Logger::DEBUG);
    Client zkClient;
    if (!zkClient.init(argv[1], atoi(argv[2]))) 
    {
        fprintf(stderr, "ZKClient failed to init...\n");
        return -1;
    }

    bool ok = zkClient.get("/test", testGet, true);
    if (!ok) 
    {
        fprintf(stderr, "ZKClient failed to GetNode...\n");
        return -1;
    }

    ok = zkClient.getChildren("/test", testGetChildren, true);
    if (!ok) 
    {
        fprintf(stderr, "ZKClient failed to GetChildren...\n");
        return -1;
    }

    ok = zkClient.exist("/test", testExist, true);
    if (!ok) 
    {
        fprintf(stderr, "ZKClient failed to Exist...\n");
        return -1;
    }

    ok = zkClient.createEphemeralSequential("/test/node-", "es", testCreate);
    if (!ok) 
    {
        fprintf(stderr, "ZKClient failed to Create...\n");
        return -1;
    }

    ok = zkClient.createEphemeral("/test/node1", "e", testCreate);
    if (!ok)
    {
        fprintf(stderr, "ZKClient failed to Create...\n");
        return -1;
    }

    /*ok = zkClient.set("/test", "set by zkclient", testSet);
    if (!ok) 
    {
        fprintf(stderr, "ZKClient failed to Set...\n");
        return -1;
    }

    ok = zkClient.del("/test", testDelete);
    if (!ok) 
    {
        fprintf(stderr, "ZKClient failed to Delete...\n");
        return -1;
    }*/

    std::string line;
    std::getline(std::cin, line);
    return 0;
}
