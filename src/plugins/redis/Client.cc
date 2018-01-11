#include "plugins/redis/Client.h"
#include "base/StringUtil.h"
#include "base/Logging.h"

#include <hiredis.h>

#include <sstream>

#include <strings.h>

#include <inttypes.h>

namespace redis
{

class ReplyHelper : boost::noncopyable
{
public:
    ReplyHelper(redisReply* reply) : reply_(reply) {}
    ~ReplyHelper()
    {
        if (reply_)
        {
            freeReplyObject(reply_);
        }        
    }

private:
    redisReply* reply_;
};

bool Status::ok() const
{
    assert(ctx_ != NULL);
    return ctx_->err == REDIS_OK;
}

bool Status::connected() const
{
    assert(ctx_ != NULL);
    return (ctx_->err & REDIS_CONNECTED);
}
   
std::string Status::errstr() const
{
    assert(ctx_ != NULL);
    return std::string(ctx_->errstr);
}

Client::Client()
    : ctx_(NULL)
{
}
    
Client::~Client()
{
    if (ctx_ != NULL)
    {
        redisFree(ctx_);
    }
}

bool Client::connect(const std::string &ip, int port)
{
    ctx_ = redisConnect(ip.c_str(), port);
    if (ctx_ == NULL || ctx_->err)
    {
        if (ctx_)
        {
            LOG_ERROR << "Connection error: " << ctx_->errstr;
        }
        else 
        {
            LOG_ERROR << "Connection error: can't allocate redis context";
        }

        return false;
    }

    ip_ = ip;
    port_ = port;
    return true;
}

bool Client::ping()
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "PING"));
    if (reply == NULL)
    {
        LOG_ERROR << "redis connection disconnected [" << ip_ << ":" << port_ << "]";
        return false;
    }

    ReplyHelper replyHelper(reply);
    if (reply->type != REDIS_REPLY_STATUS)
    {
        LOG_ERROR << "ping return type: " << reply->type << "[" << ip_ << ":" << port_ << "]";
        return false;
    }

    if (strncasecmp("PONG", reply->str, reply->len) == 0)
    {
        return true;
    }
    else
    {
        LOG_ERROR << "redis connection disconnected [" << ip_ << ":" << port_ << "]";
        return false;
    }
}

Status Client::expire(const std::string& key, int ttl)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "EXPIRE %s %d", key.c_str(), ttl));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::del(const std::string& key)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %s", key.c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::set(const std::string& key, const std::string& val)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SET %s %s", key.c_str(), val.c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::setex(const std::string& key, const std::string& val, int ttl)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SETEX %s %d %s", key.c_str(), ttl, val.c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::get(const std::string& key, std::string* val)
{
    assert(ctx_ != NULL);
    assert(val != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "GET %s", key.c_str()));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_STRING)
        {
            val->assign(reply->str, reply->len);
        }
        
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::incr(const std::string& key, int64_t incrby, int64_t* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "INCRBY %s %"PRId64, key.c_str(), incrby));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *ret = reply->integer;
        }

        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hset(const std::string& key, const std::string& field, const std::string& val)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HSET %s %s %s", key.c_str(), field.c_str(), val.c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hget(const std::string& key, const std::string& field, std::string* val)
{
    assert(ctx_ != NULL);
    assert(val != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HGET %s %s", key.c_str(), field.c_str()));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_STRING)
        {
            val->assign(reply->str, reply->len);
        }
       
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hdel(const std::string& key, const std::string& field)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HDEL %s %s", key.c_str(), field.c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hincr(const std::string& key, const std::string& field, int64_t incrby, int64_t* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HINCRBY %s %s %"PRId64, key.c_str(), field.c_str(), incrby));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *ret = reply->integer;
        }
        
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hlen(const std::string& key, int64_t* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "HLEN %s", key.c_str()));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *ret = reply->integer;
        }
        
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hclear(const std::string& key)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "DEL %s", key.c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hkeys(const std::string& key, std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "HKEYS %s", key.c_str()));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            ret->clear();
            for (size_t i = 0; i < reply->elements; i++)
            {
                ret->push_back(std::string(reply->element[i]->str, reply->element[i]->len));
            }
        }
       
        freeReplyObject(reply);        
    }

    return Status(ctx_);
}

Status Client::hscan(const std::string& key,
                          uint64_t cursor,
                          const std::string& pattern,
                          uint64_t count,
                          uint64_t* nextCursor,
                          std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(nextCursor != NULL);
    assert(ret != NULL);
    std::ostringstream oss;
    oss << "HSCAN " << cursor;
    if (!pattern.empty())
    {
        oss << " MATCH " << pattern;
    }

    oss << " COUNT " << count;
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, oss.str().c_str()));
    if (reply != NULL)
    {  
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            ret->clear();
            if (reply->elements != 2)
            {
                LOG_ERROR << oss.str();
            }

            *nextCursor = base::StringUtil::strToUInt64(
                std::string(reply->element[0]->str, reply->element[0]->len));
            for (size_t i = 0; i < reply->element[1]->elements; i++)
            {
                ret->push_back(std::string(
                    reply->element[1]->element[i]->str, reply->element[1]->element[i]->len));
            }
        }
        
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hmset(const std::string& key, const std::map<std::string, std::string>& fvs)
{
    assert(ctx_ != NULL);
    std::ostringstream oss;
    std::map<std::string, std::string>::const_iterator it;
    for (it = fvs.begin(); it != fvs.end(); ++it)
    {
        oss << " " << it->first << " " << it->second;
    }
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "HMSET %s %s", key.c_str(), oss.str().c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hmget(const std::string& key,
                          const std::vector<std::string>& fields,
                          std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    std::ostringstream oss;
    std::vector<std::string>::const_iterator it;
    for (it = fields.begin(); it != fields.end(); ++it)
    {
        oss << *it << " ";
    }
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "HMGET %s %s", key.c_str(), oss.str().c_str()));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (size_t i = 0; i < reply->elements; i++)
            {
                ret->push_back(std::string(reply->element[i]->str, reply->element[i]->len));
            }
        }
        
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::hmdel(const std::string& key, const std::vector<std::string>& fields)
{
    assert(ctx_ != NULL);
    std::ostringstream oss;
    std::vector<std::string>::const_iterator it;
    for (it = fields.begin(); it != fields.end(); ++it)
    {
        oss << *it << " ";
    }
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "DEL %s %s", key.c_str(), oss.str().c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::sadd(const std::string& key, const std::string& member)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SADD %s %s", key.c_str(), member.c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::sadd(const std::string& key, const std::vector<std::string>& members)
{
    assert(ctx_ != NULL);
    std::vector<std::string>::const_iterator it;
    std::ostringstream oss;
    for (it = members.begin(); it != members.end(); ++it)
    {
        oss << *it << " ";
    }

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SADD %s %s", key.c_str(), oss.str().c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::smembers(const std::string& key, std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "SMEMBERS %s", key.c_str()));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (size_t i = 0; i < reply->elements; i++)
            {
                ret->push_back(std::string(reply->element[i]->str, reply->element[i]->len));
            }
        }

        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::sscan(const std::string& key,
                     uint64_t cursor,
                     const std::string& pattern,
                     uint64_t count,
                     uint64_t* nextCursor,
                     std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(nextCursor != NULL);
    assert(ret != NULL);
    std::ostringstream oss;
    oss << "SSCAN " << cursor;
    if (!pattern.empty())
    {
        oss << " MATCH " << pattern;
    }

    oss << " COUNT " << count;
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, oss.str().c_str()));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            ret->clear();
            if (reply->elements != 2)
            {
                LOG_ERROR << oss.str();
            }

            *nextCursor = base::StringUtil::strToUInt64(
                std::string(reply->element[0]->str, reply->element[0]->len));
            for (size_t i = 0; i < reply->element[1]->elements; i++)
            {
                ret->push_back(std::string(
                    reply->element[1]->element[i]->str, reply->element[1]->element[i]->len));
            }
        }

        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::zadd(const std::string& key, const std::string& member, int64_t score)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "ZADD %s %"PRId64" %s", key.c_str(), score, member.c_str()));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::qpush(const std::string& key, const std::string& val, int64_t* retSize)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "RPUSH %s %s", key.c_str(), val.c_str()));
    if (reply != NULL)
    {
        if (retSize != NULL && reply->type == REDIS_REPLY_INTEGER)
        {
            *retSize = reply->integer;
        }
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::qpush(const std::string& key, const std::vector<std::string>& vals, int64_t* retSize)
{
    assert(ctx_ != NULL);
    std::ostringstream oss;
    std::vector<std::string>::const_iterator it;
    for (it = vals.begin(); it != vals.end(); ++it)
    {
        oss << *it << " ";
    }
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "RPUSH %s %s", key.c_str(), oss.str().c_str()));
    if (reply != NULL)
    {
        if (retSize != NULL && reply->type == REDIS_REPLY_INTEGER)
        {
            *retSize = reply->integer;
        }

        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::qpop(const std::string& key, std::string* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "LPOP %s", key.c_str()));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_STRING)
        {
            ret->assign(reply->str, reply->len);
        }

        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::qrange(const std::string& key, int64_t start, int64_t stop, std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "LRANGE  %s %"PRId64" %"PRId64, key.c_str(), start, stop));
    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (size_t i = 0; i < reply->elements; i++)
            {
                ret->push_back(std::string(reply->element[i]->str, reply->element[i]->len));
            }
        }

        freeReplyObject(reply);
    }

    return Status(ctx_);
}

Status Client::qtrim(const std::string& key, int64_t start, int64_t stop)
{
    assert(ctx_ != NULL); 
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "LTRIM  %s %"PRId64" %"PRId64, key.c_str(), start, stop));
    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return Status(ctx_);
}

} // namespace redis
