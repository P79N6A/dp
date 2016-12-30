#include "redis_client.h"

namespace poseidon
{  
namespace util
{

void redis_command_cb(redisAsyncContext *context, void *r, void *privdata)
{
  RedisOnReply * on_reply=(RedisOnReply *)privdata;
  redisReply * reply=(redisReply *)r;
  on_reply->run(reply);
}

void timeout_event_cb(evutil_socket_t fd, short event, void *arg)  
{  
  util::Closure *closure=(util::Closure *)arg;
  closure->Run();
}

class RedisConnChangeInfo
{
  public:
    static RedisConnChangeInfo * new_instance(RedisClient * client,int client_conn_no)
    {
      RedisConnChangeInfo * rcci=new RedisConnChangeInfo(client,client_conn_no);
      return rcci;
    }
    
    virtual ~RedisConnChangeInfo()
    {
    }
    
    void get_info(RedisClient *& client,int &client_conn_no)
    {
      client=_client;
      client_conn_no=_client_conn_no;
    }
    
  protected:
    RedisConnChangeInfo(RedisClient *client,int client_conn_no)
    {
      _client=client;
      _client_conn_no=client_conn_no;
    }
    RedisClient * _client;
    int _client_conn_no;
};

void redis_disconnect_cb(const struct redisAsyncContext* context, int status)
{
  RedisConnChangeInfo * rcci=(RedisConnChangeInfo *)context->data;
  RedisClient * client;
  int client_conn_no;
  rcci->get_info(client,client_conn_no);
  delete rcci;
  client->on_disconnect(client_conn_no,status);
}

void redis_connect_cb(const struct redisAsyncContext* context, int status)
{
  RedisConnChangeInfo * rcci=(RedisConnChangeInfo *)context->data;
  RedisClient * client;
  int client_conn_no;
  rcci->get_info(client,client_conn_no);
  client->on_connect(client_conn_no,status);
}

struct RedisConnInfo
{
  RedisConnInfo()
  {
    no=-1;
    slot_start=0;
    slot_end=0;
    active=false;
    redis_async_context=NULL;
    reconnect_event=NULL;
  }
  virtual ~RedisConnInfo()
  {
    if(active)
    {
      redisAsyncFree(redis_async_context);
    }
    if(reconnect_event!=NULL)
    {
      delete reconnect_event;
    }
  }
  int no;
  string host;
  int port;
  uint16_t slot_start;
  uint16_t slot_end;
  bool active;
  redisAsyncContext *redis_async_context;
  util::TimeoutEvent *reconnect_event;
};


RedisOnReply::RedisOnReply()
{
  
}

RedisOnReply::~RedisOnReply()
{
  
}

RedisOnReply * RedisOnReply::new_instance(FuncOnRedisReply func)
{
  RedisOnReply *ror=new RedisOnReply;
  ror->_func_on_redis_reply=func;
  return ror;
}

void RedisOnReply::run(redisReply * reply)
{
  _func_on_redis_reply(reply);
  delete this;
}

typedef boost::unordered_map<int,boost::shared_ptr<RedisConnInfo> >::iterator RedisConnIter;

RedisClient::RedisClient()
{
  _is_init=false;
  _is_cluster=false;
  _init_cluster=false;
  _client_info_num=0;
  _cluster_info_event=NULL;
  _reconnect_interval=3;
  _cluster_info_interval=20;
}

RedisClient::~RedisClient()
{
  if(_cluster_info_event!=NULL)
    delete _cluster_info_event;
}

void RedisClient::init(struct event_base * base,string host,int port,bool is_cluster)
{
  if(_is_init)
    return;
  _is_init=true;
  _base=base;
  _is_cluster=is_cluster;
  boost::shared_ptr<RedisConnInfo> rci(new RedisConnInfo);
  rci->no=_client_info_num++;
  _redis_conn_infos[rci->no]=rci;
  rci->host=host;
  rci->port=port;
  connect(rci);
}



void RedisClient::on_connect(int redis_conn_no,int status)
{
  RedisConnIter iter=_redis_conn_infos.find(redis_conn_no);
  if(iter==_redis_conn_infos.end())
  {
    return;
  }
  boost::shared_ptr<RedisConnInfo> rci=iter->second;
  if (status != REDIS_OK) 
  {
    LOG_ERROR("redis connect error,%s:%d[%s]",rci->host.c_str(),rci->port,rci->redis_async_context->errstr);
    re_connect(rci);
    return;
  }
  
  rci->active=true;
  LOG_INFO("redis connect succ,%s:%d",rci->host.c_str(),rci->port);
  if(_is_cluster && !_init_cluster)
  {
    _init_cluster=true;
    get_cluster_info(rci);
  }
}

void RedisClient::on_disconnect(int redis_conn_no,int status)
{
  RedisConnIter iter=_redis_conn_infos.find(redis_conn_no);
  if(iter==_redis_conn_infos.end())
  {
    return;
  }
  boost::shared_ptr<RedisConnInfo> rci=iter->second;
  rci->active=false;
  rci->redis_async_context=NULL;
  if (status == REDIS_OK) 
  {
    LOG_INFO("redis disconnect succ,%s:%d",rci->host.c_str(),rci->port);
    return;
  }
  LOG_ERROR("redis disconnect and re_connect,%s:%d",rci->host.c_str(),rci->port);
  re_connect(rci);
}

void RedisClient::get_cluster_info(boost::shared_ptr<RedisConnInfo> rci)
{
  LOG_DEBUG("get cluster info,redis node %d %s:%d",rci->no,rci->host.c_str(),rci->port);
  RedisOnReply *on_reply=RedisOnReply::new_instance(boost::bind(&RedisClient::on_connect_cluster,this,_1));
  redisAsyncCommand(rci->redis_async_context,redis_command_cb,(void *)on_reply,"cluster slots");
}

void RedisClient::get_cluster_info()
{
  RedisConnIter conn_iter;
  for(conn_iter=_redis_conn_infos.begin();conn_iter!=_redis_conn_infos.end();conn_iter++)
  {
    if(conn_iter->second->active)
    {
      get_cluster_info(conn_iter->second);
      break;
    }
  }
  
  if(conn_iter==_redis_conn_infos.end())
  {
    set_cluster_info_event();
  }
}

void RedisClient::set_cluster_info_event()
{
  if(_cluster_info_event!=NULL)
  {
    delete _cluster_info_event;
  }
  util::Closure *cluster_closure=util::NewCallback(this,&RedisClient::get_cluster_info);
  _cluster_info_event=new util::TimeoutEvent(_base,_cluster_info_interval,0,timeout_event_cb,(void *)cluster_closure);
  _cluster_info_event->active();
}

void RedisClient::on_connect_cluster(redisReply *reply)
{ 
  set_cluster_info_event();
  
  
  if(reply==NULL)
    return;
  if(reply->type!=REDIS_REPLY_ARRAY)
  {
    LOG_ERROR("command[cluster slots] error,reply type error[%s]",reply->str);
    return;
  }
  map<int,boost::shared_ptr<RedisConnInfo> > redis_conn_infos;
  stringstream cluster_info_string;
  for (int i = 0; i < reply->elements; i++) 
  {  
    boost::shared_ptr<RedisConnInfo> rci(new RedisConnInfo);
    redisReply* node_reply = reply->element[i];  
    if(node_reply->type==REDIS_REPLY_ARRAY)
    {
      for (int j = 0; j < node_reply->elements; j++) 
      {
        redisReply* content_reply = node_reply->element[j];
        if(content_reply->type==REDIS_REPLY_INTEGER)
        {
          if(j==0)
          {
            rci->slot_start=content_reply->integer;
          }
          else if(j==1)
          {
            rci->slot_end=content_reply->integer;
          }
          else
          {
          }
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
              {
                rci->host=serv_reply->str;
              }
            }
            else if(serv_reply->type==REDIS_REPLY_INTEGER)
            {
              if(k==1)
              {
                rci->port=serv_reply->integer;
              }
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
      if(rci->slot_end>0)
      {
        redis_conn_infos[rci->slot_end]=rci;
      }
    }
  }
  
  map<int,boost::shared_ptr<RedisConnInfo> >::iterator iter;
  for(iter=redis_conn_infos.begin();iter!=redis_conn_infos.end();iter++)
  {
    cluster_info_string<<iter->second->host<<"|"<<iter->second->port<<"|"<<iter->second->slot_start<<"|"<<iter->second->slot_end<<"|";
  }
  
  if(cluster_info_string.str().compare(_cluster_info_string)!=0)
  {
    boost::unordered_map<int,boost::shared_ptr<RedisConnInfo> > dead_conn_infos;
    if(redis_conn_infos.size()>0)
    {
      LOG_INFO("reflesh cluster map");
      dead_conn_infos=_redis_conn_infos;
      _redis_conn_infos.clear();
      _cluster_info_string=cluster_info_string.str();
      for(iter=redis_conn_infos.begin();iter!=redis_conn_infos.end();iter++)
      {
        boost::shared_ptr<RedisConnInfo> rci=iter->second;
        rci->no=_client_info_num++;
        _redis_conn_infos[rci->no]=rci;
        connect(rci);
        for(int slot_no=rci->slot_start;slot_no<=rci->slot_end;slot_no++)
        {
          _redis_slot_map[slot_no]=rci->no;
        }
      }
    }
    dead_conn_infos.clear();
  }
}

redisAsyncContext * RedisClient::get_redis_context(const string &key)
{
  redisAsyncContext * redis_async_context=NULL;
  boost::shared_ptr<RedisConnInfo> rci;
  while(_is_init)
  {
    if(!_is_cluster)
    {
      RedisConnIter iter=_redis_conn_infos.find(0);
      if(iter==_redis_conn_infos.end())
        break;
      rci=iter->second;
      if(!rci->active)
        break;
      redis_async_context=rci->redis_async_context;
    }
    else
    {
      uint16_t crc_num=util::Func::crc16(key.c_str(),key.length());
      uint16_t slot_num=crc_num%16384;
      boost::unordered_map<int,int>::iterator slot_iter=_redis_slot_map.find(slot_num);
      if(slot_iter==_redis_slot_map.end())
        break;
      RedisConnIter iter=_redis_conn_infos.find(slot_iter->second);
      if(iter==_redis_conn_infos.end())
        break;
      rci=iter->second;
      if(!rci->active)
        break;
      redis_async_context=rci->redis_async_context;
    }
    break;
  }
  return redis_async_context;
}


void RedisClient::connect(boost::shared_ptr<RedisConnInfo> rci)
{
  LOG_INFO("connect to %s:%d",rci->host.c_str(),rci->port);
  
  rci->redis_async_context=redisAsyncConnect(rci->host.c_str(),rci->port);
  if(rci->redis_async_context==NULL)
  {
    LOG_ERROR("redis connect error,%s:%d",rci->host.c_str(),rci->port);
    return;
  }
  if(rci->redis_async_context->err)
  {
    LOG_ERROR("redis connect error,%s:%d[%s]",rci->host.c_str(),rci->port,rci->redis_async_context->errstr);
    return;
  } 
  rci->redis_async_context->data=(void *)RedisConnChangeInfo::new_instance(this,rci->no);
  redisLibeventAttach(rci->redis_async_context,_base);
  redisAsyncSetConnectCallback(rci->redis_async_context,redis_connect_cb);
  redisAsyncSetDisconnectCallback(rci->redis_async_context,redis_disconnect_cb);
}

void RedisClient::re_connect(boost::shared_ptr<RedisConnInfo> rci)
{
  if(rci->reconnect_event!=NULL)
  {
    delete rci->reconnect_event;
  }
  util::Closure *interval_closure=util::NewCallback(this,&RedisClient::connect,rci);
  rci->reconnect_event=new util::TimeoutEvent(_base,_reconnect_interval,0,
            timeout_event_cb,(void *)interval_closure);
  rci->reconnect_event->active();
}

}
}