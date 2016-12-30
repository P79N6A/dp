#include "redis_client.h"

DEFINE_bool(enable_redis_cluster,false,"enable_redis_cluster");
DEFINE_string(redis_host,"127.0.0.1", "redis_host");
DEFINE_int32(redis_port,6371, "redis_port");
DEFINE_int32(redis_conn_timeout,3, "redis_conn_timeout");

namespace poseidon
{  
RedisClient::~RedisClient()
{
  if(_redis_context!=NULL)
    redisFree(_redis_context);
  _redis_clients.clear();
}

bool RedisClient::init()
{
  _host=FLAGS_redis_host;
  _port=FLAGS_redis_port;
  return real_init();
}

bool RedisClient::init(string host,int port,uint16_t slot_start,uint16_t slot_end)
{
  _host=host;
  _port=port;
  _slot_start=slot_start;
  _slot_end=slot_end;
  return real_init();
}

bool RedisClient::real_init()
{
  struct timeval tv;
  tv.tv_sec=FLAGS_redis_conn_timeout;
  tv.tv_usec=0;
  
  _redis_context=redisConnectWithTimeout(_host.c_str(),_port,tv);
  
  if(_redis_context==NULL)
  {
    cout<<"redis connect error,"<<_host<<":"<<_port<<endl;
    return false;
  }
  if(_redis_context->err)
  {
    cout<<"redis connect error["<<_redis_context->err<<"],"<<_host<<":"<<_port<<endl;
    return false;
  }
  
  
  if(FLAGS_enable_redis_cluster && _slot_end==0)
  {
    redisReply* reply=(redisReply*)redisCommand(_redis_context, "cluster slots");
    ScopeReply sr(reply);
    if (NULL == reply) 
    {  
      cout<<"command[cluster slots] error,reply is null"<<endl;
      return false;
    } 
    if(reply->type!=REDIS_REPLY_ARRAY)
    {
      cout<<"command[cluster slots] error,reply type error,"<<reply->type<<","<<reply->str<<endl;
      return false;
    }
    for (int i = 0; i < reply->elements; i++) 
    {  
      boost::shared_ptr<RedisClient> redis_client(new RedisClient);
      string host;
      int port=0;
      int slot_start=0;
      int slot_end=0;
      redisReply* node_reply = reply->element[i];  
      if(node_reply->type==REDIS_REPLY_ARRAY)
      {
        for (int j = 0; j < node_reply->elements; j++) 
        {
          redisReply* content_reply = node_reply->element[j];
          if(content_reply->type==REDIS_REPLY_INTEGER)
          {
            if(j==0)
              slot_start=content_reply->integer;
            else if(j==1)
              slot_end=content_reply->integer;
          }
          else if(content_reply->type==REDIS_REPLY_ARRAY)
          {
            if(j>2)
                continue;
            for (int k = 0; k < content_reply->elements; k++) 
            {
              redisReply* serv_reply=content_reply->element[k];
              if(serv_reply->type==REDIS_REPLY_STRING)
              {
                if(k==0)
                  host=serv_reply->str;
              }
              else if(serv_reply->type==REDIS_REPLY_INTEGER)
              {
                if(k==1)
                  port=serv_reply->integer;
              }
              else
              {
              }
            }
          }
          else
          {
            
          }
        }
      }
      if(redis_client->init(host,port,slot_start,slot_end))
      {
        for(uint16_t index=slot_start;index<=slot_end;index++)
        _redis_clients[index]=redis_client;
      }
    }
    
  }
  
  return true;
}

redisContext * RedisClient::get_redis_context(const string &key)
{
  if(_redis_clients.size()==0)
  {
    return _redis_context;
  }
  else
  {
    uint16_t crc_num=util::Func::crc16(key.c_str(),key.length());
    uint16_t slot_num=crc_num%16384;
    boost::unordered_map<uint16_t,boost::shared_ptr<RedisClient> >::iterator iter=_redis_clients.find(slot_num);
    if(iter!=_redis_clients.end())
    {
      return iter->second->get_redis_context(key);
    }
  }
  return NULL;
}
}