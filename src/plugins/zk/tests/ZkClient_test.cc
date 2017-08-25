#include "plugins/zk/ZkClient.h"
#include "base/Logging.h"
#include "base/StringUtil.h"

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        LOG_FATAL << "Usage: " << argv[0] << " ip:port timeout";
    }

    zk::ZkClient zkClient;
    zkClient.init(argv[1], base::StringUtil::strToUInt32(argv[2]));
    LOG_INFO << "createPersistentNode /test";
    zkClient.createPersistentNode("/test", "");
    std::string data;
    zkClient.readNode("/test", false, &data);
    LOG_INFO << "readNode /test: " << data;

    LOG_INFO << "createEphemeralNode /test/enode 123";
    zkClient.createEphemeralNode("/test/enode", "123");
    zkClient.readNode("/test/enode", false, &data);
    LOG_INFO << "readNode /test/enode: " << data;

    LOG_INFO << "createEphemeralSequentialNode /test/esnode 123";
    std::string path;
    zkClient.createEphemeralSequentialNode("/test/esnode", "123", &path);
    LOG_INFO << "sequential node: " << path;
    /*std::string data;
    zkClient.readNode("/test/esnode", false, &data);
    LOG_INFO << "readNode /test/esnode: " << data;*/

    std::vector<std::string> childList;
    std::vector<std::string> dataList;
    zkClient.getChildren("/test", true, &childList, &dataList);
    for (size_t i = 0; i < childList.size(); i++)
    {
        LOG_INFO << childList[i] << ": " << dataList[i];
    }
    
    std::string input;
    std::getline(std::cin, input);
    return 0;
}
