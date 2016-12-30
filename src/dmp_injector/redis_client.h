#pragma once

#include "injector_inc.h"

namespace poseidon
{  
class ScopeReply
{
  public:
    ScopeReply(redisReply * r)
    {
      reply=r;
    }
    virtual ~ScopeReply()
    {
      if(reply!=NULL)
        freeReplyObject(reply);
    }
  private:
    redisReply *reply;
};

class RedisClient
{
  public:
    RedisClient()
    {
      _redis_context=NULL;
      _slot_start=0;
      _slot_end=0;
    }
    virtual ~RedisClient();
    bool init();
    redisContext * get_redis_context(const string &key);
  protected:
    boost::unordered_map<uint16_t,boost::shared_ptr<RedisClient> > _redis_clients;
    redisContext *_redis_context;
    string _host;
    int _port;
    uint16_t _slot_start;
    uint16_t _slot_end;
  protected:
    bool real_init();
    bool init(string host,int port,uint16_t slot_start,uint16_t slot_end);
};

}