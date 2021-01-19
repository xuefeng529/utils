#include "plugins/curl/HttpClient.h"
#include "plugins/curl/MultiHttpClient.h"
#include "base/StringUtil.h"
#include "base/Logging.h"

#include <curl/curl.h>

#include <map>
#include <iostream>

///// curl -X POST https://10.207.253.123:443/api/account/bindings -H "Content-Type:application/json" -k -d "{\"from\":\"1561087283710\",\"size\":\"10\"}" 
//using namespace plugins::curl;
//
//void parse(const std::string& cmd, std::map<std::string, std::string>* kvs)
//{
//    std::vector<std::string> items;
//    base::StringUtil::split(cmd, " ", &items);
//    for (size_t i = 0; i < items.size(); i+=2)
//    {
//        kvs->insert(std::make_pair(items[i], items[i + 1]));
//    }
//}
//
//void testMulti()
//{
//	MultiHttpClient cli(true);
//	std::vector<MultiHttpClient::Request> reqs;
//	for (int i = 0; i < 3; i++)
//	{
//		MultiHttpClient::Request req;
//		req.url = "http://172.16.56.27:9999/";
//		reqs.push_back(req);
//	}
//
//	std::vector<MultiHttpClient::Result> results;
//	if (!cli.post(reqs, &results))
//	{
//		LOG_ERROR << "post failed: " << cli.strerror();
//	}
//	else
//	{
//		for (size_t i = 0; i < reqs.size(); ++i)
//		{
//			LOG_INFO << "post success, response: " << results[i].response;
//		}
//	}
//
//	std::string line;
//	std::getline(std::cin, line);
//}

int main(int argc, char* argv[])
{   
//	CURL *curl;
//	CURLcode res;
//
//	curl_global_init(CURL_GLOBAL_DEFAULT);
//
//	curl = curl_easy_init();
//	if (curl) {
//		curl_easy_setopt(curl, CURLOPT_URL, "https://10.207.253.123:443/api/account/bindings");
//		curl_easy_setopt(curl, CURLOPT_POST, 1);
//		const char *pPost = "{\"from\":\"1561087283710\",\"size\":\"10\"}";
//		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(pPost));
//		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pPost);
//		//跳过对ca的检查，简单
//#define  SKIP_PEER_VERIFICATION
//#ifdef SKIP_PEER_VERIFICATION
//		/*
//		* If you want to connect to a site who isn't using a certificate that is
//		* signed by one of the certs in the CA bundle you have, you can skip the
//		* verification of the server's certificate. This makes the connection
//		* A LOT LESS SECURE.
//		*
//		* If you have a CA cert for the server stored someplace else than in the
//		* default bundle, then the CURLOPT_CAPATH option might come handy for
//		* you.
//		*/
//		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
//#endif
//		//跳过检查
//#define SKIP_HOSTNAME_VERIFICATION
//#ifdef SKIP_HOSTNAME_VERIFICATION
//		/*
//		* If the site you're connecting to uses a different host name that what
//		* they have mentioned in their server certificate's commonName (or
//		* subjectAltName) fields, libcurl will refuse to connect. You can skip
//		* this check, but this will make the connection less secure.
//		*/
//		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
//#endif
//
//		/* Perform the request, res will get the return code */
//		res = curl_easy_perform(curl);
//		/* Check for errors */
//		if (res != CURLE_OK)
//			fprintf(stderr, "curl_easy_perform() failed: %s\n",
//			curl_easy_strerror(res));
//
//		/* always cleanup */
//		curl_easy_cleanup(curl);
//	}
//
//	curl_global_cleanup();
//
//	getchar();
//	return 0;
	////testMulti();
	plugins::curl::HttpClient cli(true);
	//cli.setCaFile(argv[2]);
	//cli.setCertFile("", "", argv[3]);
	cli.setHeader("Content-type", "application/json");
	cli.enableHttps();
	/*bool https = (atoi(argv[2]) != 0) ? true : false;
	if (https)
	{
		LOG_INFO << "supported https";
		cli.enableHttps();
	}*/
	
	std::string body = "{\"from\":\"1\",\"size\":\"10\"}";
	std::string reply;
	while (!cli.post("https://10.207.253.123:443/api/account/bindings", body, 10, &reply))
	{
		std::cout << "pull passports failed, from: " << 1561087283710 << ", body: " << body
			<< ", error: " << cli.strerror() << std::endl;
		sleep(3);
	}

	std::cout << body << std::endl;
	std::cout << reply << std::endl;
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
