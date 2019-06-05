#include "plugins/curl/HttpClient.h"
#include "plugins/curl/MultiHttpClient.h"
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

void testMulti()
{
	MultiHttpClient cli(true);
	std::vector<MultiHttpClient::Request> reqs;
	for (int i = 0; i < 3; i++)
	{
		MultiHttpClient::Request req;
		req.url = "http://172.16.56.27:9999/";
		reqs.push_back(req);
	}

	std::vector<MultiHttpClient::Result> results;
	if (!cli.post(reqs, &results))
	{
		LOG_ERROR << "post failed: " << cli.strerror();
	}
	else
	{
		for (size_t i = 0; i < reqs.size(); ++i)
		{
			LOG_INFO << "post success, response: " << results[i].response;
		}
	}

	std::string line;
	std::getline(std::cin, line);
}

int main(int argc, char* argv[])
{   
	testMulti();
    /*HttpClient cli(true);
    std::string line;    
    while (std::getline(std::cin, line))
    {         
        std::map<std::string, std::string> kvs;
        parse(line, &kvs);
        std::string url = kvs["-u"];
        int64_t timeout = atoll(kvs["-t"].c_str());
        std::string response;
        if (kvs["-x"] == "get")
        {
            cli.get(url, timeout, &response);
            LOG_INFO << "get response: " << response;
        }
        else if (kvs["-x"] == "put")
        {
            cli.put(url, kvs["-d"], timeout, &response);
            LOG_INFO << "put response: " << response;
        }
        else if (kvs["-x"] == "post")
        {
            cli.post(url, kvs["-d"], timeout, &response);
            LOG_INFO << "post response: " << response;
        }
        else if (kvs["-x"] == "delete")
        {
            cli.del(url, timeout, &response);
            LOG_INFO << "del response: " << response;
        }
        else
        {
            LOG_WARN << "unknow url: " << url;
        }
    }*/

    return 0;
}
