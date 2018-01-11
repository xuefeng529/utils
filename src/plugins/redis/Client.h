﻿#ifndef PLUGINS_REDIS_CLIENT_H
#define PLUGINS_REDIS_CLIENT_H

#include <string>
#include <vector>
#include <map>

#include <boost/noncopyable.hpp>

#include <stdint.h>

struct redisContext;

namespace redis
{

class Status
{
public:
    Status(const redisContext* ctx) : ctx_(ctx) {}
    bool ok() const;
    bool connected() const;
    std::string errstr() const;

private:
    const redisContext* ctx_;
};

/// Note: Client not thread-safe
class Client : boost::noncopyable
{
public:
    Client();
    ~Client();

    bool connect(const std::string &ip, int port);
    bool ping();

    /// Key
    /// @ttl 秒
    Status expire(const std::string& key, int ttl);
    Status del(const std::string& key);

    /// String
    Status set(const std::string& key, const std::string& val);
    Status setex(const std::string& key, const std::string& val, int ttl);
    Status get(const std::string& key, std::string* val);
    Status incr(const std::string& key, int64_t incrby, int64_t* ret);

    /// Hash
    Status hset(const std::string& key, const std::string& field, const std::string& val);
    Status hget(const std::string& key, const std::string& field, std::string* val);   
    Status hdel(const std::string& key, const std::string& field);
    Status hincr(const std::string& key, const std::string& field, int64_t incrby, int64_t* ret);
    Status hlen(const std::string& key, int64_t* ret);
    Status hclear(const std::string& key);    
    Status hkeys(const std::string& key, std::vector<std::string>* ret);
    /// @cursor 0开始一轮新的迭代
    /// @pattern 为空时不进行模式匹配
    /// @nextCursor 返回0表示本轮迭代完成，否则用返回值继续迭代
    /// @ret 返回field,value对
    Status hscan(const std::string& key,
                 uint64_t cursor,
                 const std::string& pattern,
                 uint64_t count,
                 uint64_t* nextCursor,
                 std::vector<std::string>* ret);
    Status hmset(const std::string& key, const std::map<std::string, std::string>& fvs);
    Status hmget(const std::string& key, 
                 const std::vector<std::string>& fields,
                 std::vector<std::string>* ret); 
    Status hmdel(const std::string& key, const std::vector<std::string>& fields);

    Status multi_zget(const std::string &name, const std::vector<std::string> &keys,
                      std::vector<std::string> *scores);
    
    /// Set
    Status sadd(const std::string& key, const std::string& member);
    Status sadd(const std::string& key, const std::vector<std::string>& members);
    Status smembers(const std::string& key, std::vector<std::string>* ret);
    /// @cursor 0开始一轮新的迭代
    /// @pattern 为空时不进行模式匹配
    /// @nextCursor 返回0表示本轮迭代完成，否则用返回值继续迭代
    /// @ret 返回集合中的元素
    Status sscan(const std::string& key,
                 uint64_t cursor,
                 const std::string& pattern,
                 uint64_t count,
                 uint64_t* nextCursor,
                 std::vector<std::string>* ret);

    /// SortedSet
    Status zadd(const std::string& key, const std::string& member, int64_t score);

    /// List
    Status qpush(const std::string& key, const std::string& val, int64_t* retSize = NULL);
    Status qpush(const std::string& key, const std::vector<std::string>& vals, int64_t* retSize = NULL);
    Status qpop(const std::string& key, std::string* ret);
    Status qrange(const std::string& key, int64_t start, int64_t stop, std::vector<std::string>* ret);
    Status qtrim(const std::string& key, int64_t start, int64_t stop);

private:
    redisContext *ctx_;
    std::string ip_;
    int port_;
};

} // namespace redis

#endif // PLUGINS_REDIS_CLIENT_H