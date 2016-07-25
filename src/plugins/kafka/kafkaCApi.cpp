#include "kafkaCApi.h"
#include "string.h"
#include "plugins/json/json.h"
#include "base/Logging.h"
#include <boost/bind.hpp>

#pragma GCC diagnostic ignored "-Wold-style-cast"

KafkaApi::KafkaApi(string& zk_host,string& topic_path,string& ids_path,BoostFuncCallback& boostfunc,int offset) : m_boostfunc(boostfunc),m_zk_host(zk_host),m_topic_path(topic_path),m_ids_path(ids_path),m_offset(offset),m_runflag(true)
{
    m_brokers = "";
}

KafkaApi::~KafkaApi()
{
    rd_kafka_topic_destroy(m_rkt);

    list<Thread*>::iterator it; 
    for(it = m_list_thread.begin();it!=m_list_thread.end();it++)
    {   
        delete *it;
        if(*it != NULL)
            (*it) = NULL;
    }
}

void msg_consume (rd_kafka_message_t *rkmessage,void* opaque) 
{
    if (rkmessage->err) 
    {   
        if (rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) 
        {   
            LOG_INFO<<"Consumer reached end,partition="<<rkmessage->partition<<",offset="<<rkmessage->offset;
            return;
        }   

        LOG_INFO<<"Consume error="<<rd_kafka_message_errstr(rkmessage);
        return;
    }  

    KafkaApi* m_api = (KafkaApi*)opaque;
    if(m_api == NULL)
    {
        LOG_ERROR<<"kafkaapi object is null";
        return;
    }
    m_api->m_boostfunc((char*)rkmessage->payload,rkmessage->len);
}

void KafkaApi::StartKafkaApi()
{
    map<string,int>::iterator it;
    for(it = map_kfk_info.begin();it!=map_kfk_info.end();it++)
    {
        boost::function<void()> f;
        f = boost::bind(&KafkaApi::FuncDelivered,this,it->second);    
        Thread* m_thread = new  Thread(f, "ThreadConsumer");
        m_thread->start();
        m_list_thread.push_back(m_thread);
    }

    Join();
}

void KafkaApi::FuncDelivered(int partition)
{
    if (rd_kafka_consume_start(m_rkt, partition, m_offset) == -1)
    {
        LOG_ERROR<<"Failed to start consuming:"<<rd_kafka_err2str(rd_kafka_last_error());
    }
    while(m_runflag)
    {
        int ret = rd_kafka_consume_callback(m_rkt, partition,1000,msg_consume,this);
        if(ret == -1) 
            LOG_ERROR<<"Error:"<<rd_kafka_err2str(rd_kafka_last_error());
    }   
    rd_kafka_consume_stop(m_rkt, partition);
    return ;
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

bool KafkaApi::Start()
{
    GetKafkaMetaDataByZKP();

    m_conf = rd_kafka_conf_new();
    m_topic_conf = rd_kafka_topic_conf_new();

    rd_kafka_conf_set(m_conf, "queued.min.messages", "1000000", NULL, 0);
    rd_kafka_topic_conf_set(m_topic_conf, "message.timeout.ms", "5000",NULL, 0);

    if (!(m_rk = rd_kafka_new(RD_KAFKA_CONSUMER, m_conf,errstr, sizeof(errstr)))) 
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

void KafkaApi::Join()
{
    list<Thread*>::iterator it;
    for(it = m_list_thread.begin();it!=m_list_thread.end();it++)
    {   
        (*it)->join();
    }   
}

void KafkaApi::Stop()
{
    m_runflag = false;
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

            int port;
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
