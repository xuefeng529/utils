#include "plugins/curl/HttpClient.h"
#include "base/StringUtil.h"
#include "base/Logging.h"

#include <map>
#include <iostream>

using namespace plugins::curl;

void parse(const std::string& cmd, std::map<std::string, std::string>* kvs)
{
    std::vector<std::string> items;
    base::StringUtil::split(cmd, " ", &items);
    for (size_t i = 0; i < items.size(); i+=2)
    {
        kvs->insert(std::make_pair(items[i], items[i + 1]));
    }
}

int main(int argc, char* argv[])
{   
    HttpClient cli(true);
    std::string line;    
    while (std::getline(std::cin, line))
    {         
        std::map<std::string, std::string> kvs;
        parse(line, &kvs);
        std::string url = kvs["-u"];
        std::string response;
        if (kvs["-x"] == "get")
        {
            cli.get(url, &response);
            LOG_INFO << "get response: " << response;
        }
        else if (kvs["-x"] == "put")
        {
            cli.put(url, kvs["-d"], &response);
            LOG_INFO << "put response: " << response;
        }
        else if (kvs["-x"] == "post")
        {
            cli.post(url, kvs["-d"], &response);
            LOG_INFO << "post response: " << response;
        }
        else if (kvs["-x"] == "delete")
        {
            cli.del(url, &response);
            LOG_INFO << "del response: " << response;
        }
        else
        {
            LOG_WARN << "unknow url: " << url;
        }
    }

    return 0;
}
