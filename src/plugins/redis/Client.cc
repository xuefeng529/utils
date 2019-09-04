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

Status::Status()
	: ctx_(NULL),
	  reply_(NULL)
{
}

Status::Status(const Status& other)
	: ctx_(other.ctx_),
	  reply_(other.reply_)
{
}

Status::Status(const redisContext* ctx, const redisReply* reply) 
    : ctx_(ctx),
      reply_(reply)
{
}

Status& Status::operator=(const Status& other)
{
	if (&other != this)
	{
		ctx_ = other.ctx_;
		reply_ = other.reply_;
	}
	
	return *this;
}

bool Status::ok() const
{
    return (reply_ != NULL && reply_->type != REDIS_REPLY_ERROR);
}

bool Status::valid() const
{
    return (ctx_ != NULL && reply_ != NULL && ctx_->err == REDIS_OK);
}
   
std::string Status::errstr() const
{
    if (reply_ != NULL && reply_->type == REDIS_REPLY_ERROR)
    {
        return std::string(reply_->str, reply_->len);
    }
    else if (ctx_ != NULL)
    {
		return ctx_->errstr;
    }
	else
	{
		return "uninitialized redis context";
	}
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

bool Client::connect(const std::string& ip, int port, const std::string& password)
{
    if (ctx_ != NULL)
    {
        redisFree(ctx_);
        ctx_ = NULL;
    }

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
    password_ = password;
    Status s = auth(password);
    return s.ok();
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

Status Client::exists(const std::string& key, bool* val)
{
    assert(ctx_ != NULL);
    assert(val != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "EXISTS %s", key.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "EXISTS " << key << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *val = (reply->integer == 1 ? true : false);
        }

        freeReplyObject(reply);
    }

    return status;
}

Status Client::expire(const std::string& key, int ttl)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "EXPIRE %s %d", key.c_str(), ttl));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "EXPIRE " << key << " " << ttl << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::del(const std::string& key)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %s", key.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "DEL " << key << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::set(const std::string& key, const std::string& val)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SET %s %b", key.c_str(), val.data(), val.size()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "SET " << key << " " << val << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::setex(const std::string& key, const std::string& val, int ttl)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SETEX %s %d %b", key.c_str(), ttl, val.data(), val.size()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "SETEX " << key << " " << val << " " << ttl << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::get(const std::string& key, std::string* val)
{
    assert(ctx_ != NULL);
    assert(val != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "GET %s", key.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "GET " << key << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_STRING)
        {
            val->assign(reply->str, reply->len);
        }
        
        freeReplyObject(reply);
    }

    return status;
}

Status Client::incr(const std::string& key, int64_t incrby, int64_t* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "INCRBY %s %"PRId64, key.c_str(), incrby));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "INCRBY " << key << " " << incrby << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *ret = reply->integer;
        }

        freeReplyObject(reply);
    }

    return status;
}

Status Client::hexists(const std::string& key, const std::string& field, bool* val)
{
    assert(ctx_ != NULL);
    assert(val != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HEXISTS %s %s", key.c_str(), field.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "HEXISTS " << key << " " << field << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *val = (reply->integer == 1 ? true : false);
        }

        freeReplyObject(reply);
    }

    return status;
}

Status Client::hset(const std::string& key, const std::string& field, const std::string& val)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HSET %s %s %b", key.c_str(), field.c_str(), val.data(), val.size()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "HSET " << key << " " << field << " " << val << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::hmset(const std::string& key, const std::map<std::string, std::string>& fvs)
{
    assert(ctx_ != NULL);
    std::vector<std::string> cmd;
    cmd.push_back("HMSET");
    cmd.push_back(key);
    std::map<std::string, std::string>::const_iterator it;
    for (it = fvs.begin(); it != fvs.end(); ++it)
    {
        cmd.push_back(it->first);
        cmd.push_back(it->second);
    }

    std::string cmdStr;
    redisReply* reply = wrapCommandArgv(cmd, &cmdStr);
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << cmdStr << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::hget(const std::string& key, const std::string& field, std::string* val)
{
    assert(ctx_ != NULL);
    assert(val != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HGET %s %s", key.c_str(), field.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "HGET " << key << " " << field << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_STRING)
        {
            val->assign(reply->str, reply->len);
        }
       
        freeReplyObject(reply);
    }

    return status;
}

Status Client::hmget(const std::string& key,
                     const std::vector<std::string>& fields,
                     std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    std::vector<std::string> cmd;
    cmd.push_back("HMGET");
    cmd.push_back(key); 
    cmd.insert(cmd.end(), fields.begin(), fields.end());
    std::string cmdStr;
    redisReply* reply = wrapCommandArgv(cmd, &cmdStr);
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << cmdStr << " [" << status.errstr() << "]";
    }

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

    return status;
}

Status Client::hdel(const std::string& key, const std::string& field)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HDEL %s %s", key.c_str(), field.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "HDEL " << key << " " << field << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::hdel(const std::string& key, const std::vector<std::string>& fields)
{
    assert(ctx_ != NULL);
    std::vector<std::string> cmd;
    cmd.push_back("HDEL");
    cmd.push_back(key);
    cmd.insert(cmd.end(), fields.begin(), fields.end());
    std::string cmdStr;
    redisReply* reply = wrapCommandArgv(cmd, &cmdStr);
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << cmdStr << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::hincr(const std::string& key, const std::string& field, int64_t incrby, int64_t* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "HINCRBY %s %s %"PRId64, key.c_str(), field.c_str(), incrby));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "HINCRBY " << key << " " << field << " " << incrby << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *ret = reply->integer;
        }
        
        freeReplyObject(reply);
    }

    return status;
}

Status Client::hlen(const std::string& key, int64_t* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "HLEN %s", key.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "HLEN " << key << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *ret = reply->integer;
        }
        
        freeReplyObject(reply);
    }

    return status;
}

Status Client::hkeys(const std::string& key, std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "HKEYS %s", key.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "HKEYS " << key << " [" << status.errstr() << "]";
    }

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

    return status;
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
    std::stringstream ss;
    ss << "HSCAN " << key << " " << cursor;
    if (!pattern.empty())
    {
        ss << " MATCH " << pattern;
    }

    ss << " COUNT " << count;
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, ss.str().c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << ss.str() << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {  
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            ret->clear();
            if (reply->elements != 2)
            {
                LOG_ERROR << ss.str();
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

    return status;
}

Status Client::sadd(const std::string& key, const std::string& member)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SADD %s %b", key.c_str(), member.data(), member.size()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "SADD " << key << " " << member << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::sadd(const std::string& key, const std::vector<std::string>& members)
{
    assert(ctx_ != NULL);
    std::vector<std::string> cmd;
    cmd.push_back("SADD");
    cmd.push_back(key);
    cmd.insert(cmd.end(), members.begin(), members.end());
    std::string cmdStr;
    redisReply* reply = wrapCommandArgv(cmd, &cmdStr);
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << cmdStr << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::smembers(const std::string& key, std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "SMEMBERS %s", key.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "SMEMBERS " << key << " [" << status.errstr() << "]";
    }

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

    return status;
}

Status Client::scard(const std::string& key, uint64_t* ret)
{
	assert(ctx_ != NULL);
	assert(ret != NULL);
	redisReply* reply = static_cast<redisReply*>(
		redisCommand(ctx_, "SCARD  %s", key.c_str()));
	Status status(ctx_, reply);
	if (!status.ok())
	{
		LOG_ERROR << "SCARD " << key << " [" << status.errstr() << "]";
	}

	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_INTEGER)
		{
			*ret = reply->integer;
		}

		freeReplyObject(reply);
	}

	return status;
}

Status Client::spop(const std::string& key, std::string* ret)
{
	assert(ctx_ != NULL);
	redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, "SPOP %s", key.c_str()));
	Status status(ctx_, reply);
	if (!status.ok())
	{
		LOG_ERROR << "SPOP " << key << " [" << status.errstr() << "]";
	}

	if (reply != NULL)
	{
		if (reply->type == REDIS_REPLY_STRING)
		{
			if (ret != NULL)
			{
				ret->assign(reply->str, reply->len);
			}
		}

		freeReplyObject(reply);
	}

	return status;
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
    std::stringstream ss;
    ss << "SSCAN " << cursor;
    if (!pattern.empty())
    {
        ss << " MATCH " << pattern;
    }

    ss << " COUNT " << count;
    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx_, ss.str().c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << ss.str() << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            ret->clear();
            if (reply->elements != 2)
            {
                LOG_ERROR << ss.str();
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

    return status;
}

Status Client::zadd(const std::string& key, const std::string& member, int64_t score)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "ZADD %s %"PRId64" %s", key.c_str(), score, member.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "ZADD " << key << " " << score << " " << member << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::zadd(const std::string& key, const std::map<std::string, int64_t>& mss)
{
    assert(ctx_ != NULL);
    std::vector<std::string> fields;
    fields.push_back("ZADD");
    fields.push_back(key);
    std::map<std::string, int64_t>::const_iterator it = mss.begin();
    for (; it != mss.end(); ++it)
    {
        fields.push_back(base::StringUtil::int64ToStr(it->second));
        fields.push_back(it->first);
    }

    std::string cmd;
    redisReply* reply = wrapCommandArgv(fields, &cmd);
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << cmd << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::zrange(const std::string& key, int64_t start, int64_t stop, std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "ZRANGE %s %"PRId64" %"PRId64, key.c_str(), start, stop));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "ZRANGE " << key << " " << start << " " << stop << " [" << status.errstr() << "]";
    }

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

    return status;
}

Status Client::zcard(const std::string& key, uint64_t* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "ZCARD  %s", key.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "ZCARD " << key << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            *ret = reply->integer;
        }

        freeReplyObject(reply);
    }

    return status;
}

Status Client::zremrangebyrank(const std::string& key, uint64_t start, uint64_t stop)
{
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "ZREMRANGEBYRANK %s %"PRIu64" %"PRIu64, key.c_str(), start, stop));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "ZREMRANGEBYRANK " << key << " " << start << " " << stop << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

Status Client::qpush(const std::string& key, const std::string& val, int64_t* retSize)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "RPUSH %s %b", key.c_str(), val.data(), val.size()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "RPUSH " << key << " " << val << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (retSize != NULL && reply->type == REDIS_REPLY_INTEGER)
        {
            *retSize = reply->integer;
        }
        freeReplyObject(reply);
    }

    return status;
}

Status Client::qpush(const std::string& key, const std::vector<std::string>& vals, int64_t* retSize)
{
    assert(ctx_ != NULL);
    std::vector<std::string> cmd;
    cmd.push_back("RPUSH");
    cmd.push_back(key);
    cmd.insert(cmd.end(), vals.begin(), vals.end());
    std::string cmdStr;
    redisReply* reply = wrapCommandArgv(cmd, &cmdStr);
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << cmdStr << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (retSize != NULL && reply->type == REDIS_REPLY_INTEGER)
        {
            *retSize = reply->integer;
        }

        freeReplyObject(reply);
    }

    return status;
}

Status Client::qlpush(const std::string& key, const std::string& val, int64_t* retSize)
{
	assert(ctx_ != NULL);
	redisReply* reply = static_cast<redisReply*>(redisCommand(
		ctx_, "LPUSH %s %b", key.c_str(), val.data(), val.size()));
	Status status(ctx_, reply);
	if (!status.ok())
	{
		LOG_ERROR << "LPUSH " << key << " " << val << " [" << status.errstr() << "]";
	}

	if (reply != NULL)
	{
		if (retSize != NULL && reply->type == REDIS_REPLY_INTEGER)
		{
			*retSize = reply->integer;
		}

		freeReplyObject(reply);
	}

	return status;
}

Status Client::qlpush(const std::string& key, const std::vector<std::string>& vals, int64_t* retSize)
{
	assert(ctx_ != NULL);
	std::vector<std::string> cmd;
	cmd.push_back("LPUSH");
	cmd.push_back(key);
	cmd.insert(cmd.end(), vals.begin(), vals.end());
	std::string cmdStr;
	redisReply* reply = wrapCommandArgv(cmd, &cmdStr);
	Status status(ctx_, reply);
	if (!status.ok())
	{
		LOG_ERROR << cmdStr << " [" << status.errstr() << "]";
	}

	if (reply != NULL)
	{
		if (retSize != NULL && reply->type == REDIS_REPLY_INTEGER)
		{
			*retSize = reply->integer;
		}

		freeReplyObject(reply);
	}

	return status;
}

Status Client::qpop(const std::string& key, std::string* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "LPOP %s", key.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "LPOP " << key << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        if (reply->type == REDIS_REPLY_STRING)
        {
            ret->assign(reply->str, reply->len);
        }

        freeReplyObject(reply);
    }

    return status;
}

Status Client::qrange(const std::string& key, int64_t start, int64_t stop, std::vector<std::string>* ret)
{
    assert(ctx_ != NULL);
    assert(ret != NULL);
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "LRANGE  %s %"PRId64" %"PRId64, key.c_str(), start, stop));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "LRANGE " << key << " " << start << " " << stop << " [" << status.errstr() << "]";
    }

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

    return status;
}

Status Client::qtrim(const std::string& key, int64_t start, int64_t stop)
{
    assert(ctx_ != NULL); 
    redisReply* reply = static_cast<redisReply*>(redisCommand(
        ctx_, "LTRIM  %s %"PRId64" %"PRId64, key.c_str(), start, stop));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "LTRIM " << key << " " << start << " " << stop << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

redisReply* Client::wrapCommandArgv(const std::vector<std::string>& cmd, std::string* cmdStr)
{
    std::vector<const char*> argv(cmd.size());
    std::vector<size_t> argvLen(cmd.size());
    std::vector<std::string>::const_iterator it = cmd.begin();
    size_t i = 0;
    for (; it != cmd.end(); ++it, ++i)
    {
        if (cmdStr != NULL)
        {
            if (it != cmd.begin())
            {
                cmdStr->append(" ");
            }
            cmdStr->append(*it);
        }
        argv[i] = it->data();
        argvLen[i] = it->size();
    }

    return static_cast<redisReply*>(redisCommandArgv(ctx_, argv.size(), argv.data(), argvLen.data()));
}

Status Client::auth(const std::string& password)
{
    assert(ctx_ != NULL);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "AUTH %s", password.c_str()));
    Status status(ctx_, reply);
    if (!status.ok())
    {
        LOG_ERROR << "AUTH " << password << " [" << status.errstr() << "]";
    }

    if (reply != NULL)
    {
        freeReplyObject(reply);
    }

    return status;
}

} // namespace redis
