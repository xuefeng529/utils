#pragma once
#include <string>
#include <map>
#include <list>
#include "base/Thread.h"
#include "zookeeper.h"
extern "C" {
#include <librdkafka/rdkafka.h>
}

using namespace std;
using namespace base;

typedef boost::function<void (char*,size_t)> BoostFuncCallback;

#define OFFSET_BEGINNING -2
#define OFFSET_END       -1

class KafkaApi
{
    public : 
        KafkaApi(string& zk_host,string& topic_path,string& ids_path,BoostFuncCallback& boostfunc,int offset = OFFSET_END);
        virtual ~KafkaApi();
        bool Start();
        void Stop();
        BoostFuncCallback m_boostfunc;

    private :
        bool GetKafkaMetaDataByZKP();
        void StartKafkaApi();
        void FuncDelivered(int partition);
        void FuncDeliveredBatch(int partition);
        void Join();

    private :
        list<Thread*> m_list_thread;
        rd_kafka_topic_t *m_rkt;
        rd_kafka_t *m_rk;
        map<string,int> map_kfk_info;
        string m_brokers;
        string m_topic;
        rd_kafka_conf_t *m_conf;
        rd_kafka_topic_conf_t *m_topic_conf;
        char errstr[512];

        string m_zk_host;
        string m_topic_path;
        string m_ids_path;
        int m_offset;
        bool m_runflag;
};

class ZooApi
{
    public:
        ZooApi(string host) : host(host) 
    {   
        m_bZookeeper_connected = false;
    }   
        ~ZooApi();
        int Init();
    public:
        bool m_bZookeeper_connected;
    private:
        zhandle_t *zh;
        string host;
        void process_kfk_strings(struct String_vector *strings,map<string,int>& kfkinfo);

    private:
        string m_kfk_path;
        string m_kfk_path2;
    public:
        bool get_kafka_info(const string path_name,const string path2,map<string,int>& kfkinfo);
};
