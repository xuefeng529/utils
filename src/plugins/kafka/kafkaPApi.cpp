#include "kafkaPApi.h"
#include "plugins/json/json.h"
#include "base/Logging.h"

#include <boost/bind.hpp>

#include <string.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"

KafkaApi::KafkaApi(string& zk_host,string& topic_path,string& ids_path,BoostFuncCallback& boostfunc) : m_boostfunc(boostfunc),m_zk_host(zk_host),m_topic_path(topic_path),m_ids_path(ids_path)
{
    m_brokers = "";
}

KafkaApi::~KafkaApi()
{
    rd_kafka_topic_destroy(m_rkt);
}

static void msg_delivered (rd_kafka_t * /*rk*/,
        void * payload, size_t len,
        rd_kafka_resp_err_t error_code,
        void * /*opaque*/, void * msg_opaque) {

    if (error_code)
    {
        KafkaApi* m_api = (KafkaApi*)msg_opaque;
        m_api->m_boostfunc((char*)payload,len);
        LOG_ERROR<<"Message delivery failed:"<<rd_kafka_err2str(error_code)<<",msg:"<<payload<<",len"<<len;
    }
}

void KafkaApi::StartKafkaApi()
{
    boost::function<void()> f;
    f = boost::bind(&KafkaApi::FuncDelivered,this);    
    m_thread = new  Thread(f, "ThreadTest");
    m_thread->start();
}

void KafkaApi::FuncDelivered()
{
    while(true)
    {
        /*int len  = GetProcessQueueLen();
        if(len == 0)
            sleep(5);
        else*/
            rd_kafka_poll(m_rk, 50);
    }
}

bool KafkaApi::GetKafkaMetaDataByZKP()
{
    ZooApi* m_zkapi = new ZooApi(m_zk_host);
    if(m_zkapi->Init() != 0)
    {
        LOG_INFO<< "zookeeper init error";
        return false;
    }
    bool ret = m_zkapi->get_kafka_info(m_topic_path,m_ids_path,map_kfk_info);

    int pos = m_topic_path.find("topics") + strlen("topics") +1; 
    int end = m_topic_path.find("/",pos);
    m_topic = m_topic_path.substr(pos,end-pos);

    if(ret)
    {
        size_t i =0;
        map<string,int>::iterator it;
        for(it = map_kfk_info.begin();it!=map_kfk_info.end();it++)
        {
            m_brokers.append(it->first);
            if(i != map_kfk_info.size() -1)
                m_brokers.append(",");
            i++;
        }
    }
    else
    {
        LOG_ERROR<<"get kafka metadata error,topic_node:"<<m_topic_path<<",ids_node:"<<m_ids_path;
    }

    LOG_INFO<< "get kafka metadata,top:"<<m_topic<<",broker:"<<m_brokers;
    delete m_zkapi;
    if(!m_zkapi)
        m_zkapi = NULL;
    return ret;
}

bool KafkaApi::Init()
{

    GetKafkaMetaDataByZKP();

    m_conf = rd_kafka_conf_new();
    m_topic_conf = rd_kafka_topic_conf_new();

    rd_kafka_conf_set(m_conf, "queued.min.messages", "1000000", NULL, 0);
    rd_kafka_topic_conf_set(m_topic_conf, "message.timeout.ms", "5000",NULL, 0);

    rd_kafka_conf_set_dr_cb(m_conf, msg_delivered);

    if (!(m_rk = rd_kafka_new(RD_KAFKA_PRODUCER, m_conf,errstr, sizeof(errstr)))) 
    {
        LOG_ERROR<<"Failed to create new producer:"<<errstr;
        return false;
    }

    if (rd_kafka_brokers_add(m_rk, m_brokers.c_str()) == 0) 
    {
        LOG_ERROR<<"No valid brokers specified";
        return false;
    }

    m_rkt = rd_kafka_topic_new(m_rk, m_topic.c_str(), m_topic_conf);
    m_topic_conf = NULL; /* Now owned by topic */

    StartKafkaApi();

    return true;
}

void KafkaApi::SendKafkaQueue(char* buf, size_t len)
{
    void* pbuf = malloc(len);
    memcpy(pbuf, buf, len);
    if (rd_kafka_produce(m_rkt, RD_KAFKA_PARTITION_UA,RD_KAFKA_MSG_F_FREE,pbuf, len,NULL, 0,(void*)this) == -1) 
    {
        LOG_ERROR<<"Failed to produce to topic:"<<rd_kafka_topic_name(m_rkt)<<",info:"<<rd_kafka_err2str(rd_kafka_last_error());
    }
    rd_kafka_poll(m_rk, 0);
}

void watcher(zhandle_t *zzh, int type, int state, const char * /*path*/, void * watcherCtx)
{
    if(type == ZOO_SESSION_EVENT)
    {   
        if(state == ZOO_CONNECTED_STATE)
        {   
            const clientid_t *sessionid = zoo_client_id(zzh);
            LOG_INFO << "zookeeper connected,clientid= "<<sessionid->client_id;
            if(NULL != watcherCtx)
            {   
                ((ZooApi*)watcherCtx)->m_bZookeeper_connected = true;
            }   
        }   
        else if(state == ZOO_EXPIRED_SESSION_STATE)
        {   
            const clientid_t * sessionid = zoo_client_id(zzh);
            LOG_INFO << "zookeeper session expired, sessionid= "<<sessionid->client_id<<" close handler"<<zzh;
            if(NULL != watcherCtx)
            {   
                ((ZooApi*)watcherCtx)->m_bZookeeper_connected = false;
            }   
            zookeeper_close(zzh);
        }   
    }   
}

int ZooApi::Init()
{
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    zh = zookeeper_init(host.c_str(), watcher, 100000, 0, this, 0); 
    if(!zh)
    {   
        LOG_INFO << "zookeeper_init failed host = "<<host;
        return -1; 
    }   

    return 0;
}

ZooApi::~ZooApi()
{
    if(zh)
    {
        zookeeper_close(zh);
        zh = NULL;
    }
}

void ZooApi::process_kfk_strings(struct String_vector *strings,map<string,int>& kfkinfo)
{
    if(NULL == strings)
    {
        return;
    }

    Json::Reader reader(Json::Features::strictMode());

    for(int i = 0; i < strings->count; ++i)
    {
        string newpath = m_kfk_path + "/" + strings->data[i] + "/state";
        char buf[128];
        int len = sizeof(buf);

        string host;
        /*int pos = m_kfk_path.find("topics") + strlen("topics") +1;
         *           int end = m_kfk_path.find("/",pos);
         *                     string topic = m_kfk_path.substr(pos,end-pos);*/
        int rc = zoo_exists(zh, newpath.c_str(), 0, NULL);

        if(rc == ZOK)
        {
            int ret = zoo_get(zh,newpath.c_str(),0,buf,&len,NULL);

            string data(buf, len);
            int leader = -1;
            Json::Value root;
            if(reader.parse(data,root))
            {
                if( !root["leader"].isNull())
                {
                    leader = root["leader"].asInt();
                }
            }

            int port = 0;
            if(leader != -1)
            {
                newpath = m_kfk_path2;
                newpath.append("/");
                char sm[32];
                sprintf(sm,"%d",leader);
                newpath.append(sm,strlen(sm));

                char buf2[256];
                int len2 = sizeof(buf2);
                ret = zoo_get(zh,newpath.c_str(),0,buf2,&len2,NULL);

                string jsonstr(buf2, len2);
                Json::Value root2;
                if(reader.parse(jsonstr,root2))
                {
                    if( !root2["host"].isNull())
                    {
                        host = root2["host"].asString();
                    }
                    if( !root2["port"].isNull())
                    {
                        port = root2["port"].asInt();
                    }
                }
            }
            char pstr[64];
            sprintf(pstr,"%d",port);
            host.append(":",1);
            host.append(pstr,strlen(pstr));

            int paitition = atoi(strings->data[i]);
            kfkinfo.insert(make_pair(host,paitition));
        }
    }
    deallocate_String_vector(strings);
    return;
}

bool ZooApi::get_kafka_info(const string path_name,const string path2,map<string,int>& kfkinfo)
{
    if(path_name[0] != '/')
    {
        m_kfk_path = "/" + path_name;
    }
    else
    {
        m_kfk_path = path_name;
    }

    if(path2[0] != '/')
    {
        m_kfk_path2 = "/" + path2;
    }
    else
    {
        m_kfk_path2 = path2;
    }

    struct String_vector strings;
    int iRet = zoo_get_children(zh,m_kfk_path.c_str(),0,&strings);
    if(ZOK == iRet)
    {
        process_kfk_strings(&strings,kfkinfo);
        deallocate_String_vector(&strings);
        return true;
    }
    else
    {
        LOG_INFO << "get_kafka_path(): path = "<<m_kfk_path<<" result = "<<iRet;
        return false;
    }
}
