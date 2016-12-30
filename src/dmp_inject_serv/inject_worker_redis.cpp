#include "inject_worker.h"
namespace poseidon
{
namespace inject
{


void HttpWorker::get_user_tags(boost::shared_ptr<InjectContext> context)
{
  redisAsyncContext * redis_context=_redis_client.get_redis_context(context->get_bucket_key());
  if(redis_context==NULL)
  {
    LOG_ERROR("%s get redis context error",context->uid.c_str());
    context->set_http_internal_err();
    http_done(context);
    return;
  }
  util::RedisOnReply *on_reply=
  util::RedisOnReply::new_instance(boost::bind(&HttpWorker::on_got_user_tags,this,context->id,_1));
  if(redisAsyncCommand(redis_context, util::redis_command_cb, (void *)on_reply,
        "hget %s %s",context->get_bucket_key().c_str(),context->get_bucket_index().c_str())!=0)
  {
    LOG_ERROR("%s redis command error",context->uid.c_str());
    context->set_http_internal_err();
    http_done(context);
    return;
  }
  else
  {
    
  }
}

void HttpWorker::on_got_user_tags(uint64_t context_id,redisReply *reply)
{
  ContextIter iter=_context_map.find(context_id);
  if(iter==_context_map.end())
    return;
  boost::shared_ptr<InjectContext> context=iter->second;
  if(reply==NULL)
  {
  }
  else if(reply->type == REDIS_REPLY_STRING)
  {
    if(context->dmp_user_data.ParseFromArray(reply->str,reply->len))
    {
      LOG_DEBUG("dmp_user_data : \r\n %s",context->dmp_user_data.DebugString().c_str());
    }
    else
    {
      LOG_ERROR("%s redis data parse error",context->uid.c_str());
      context->set_http_internal_err();
    }
  }
  else if(reply->type == REDIS_REPLY_NIL)
  {
  }
  else
  {
    LOG_ERROR("%s redis reply error,%d",context->uid.c_str(),reply->type);
    context->set_http_internal_err();
  }
  ++context->prog;
  schedule(context);
}

void HttpWorker::set_user_tag(boost::shared_ptr<InjectContext> context)
{
  redisAsyncContext * redis_context=_redis_client.get_redis_context(context->get_bucket_key());
  if(redis_context==NULL)
  {
    LOG_ERROR("%s get redis context error",context->uid.c_str());
    context->set_http_internal_err();
    http_done(context);
    return;
  }
  
  char buffer[1024*128]={0};
  context->dmp_user_data.SerializeToArray(buffer,sizeof(buffer));
  
  util::RedisOnReply *on_reply=
  util::RedisOnReply::new_instance(boost::bind(&HttpWorker::on_set_user_tag,this,context->id,_1));
  if(redisAsyncCommand(redis_context, util::redis_command_cb, (void *)on_reply,
        "hset %s %s %b",context->get_bucket_key().c_str(),context->get_bucket_index().c_str(),
        buffer,context->dmp_user_data.ByteSize())!=0)
  {
    LOG_ERROR("%s redis command error",context->uid.c_str());
    context->set_http_internal_err();
    http_done(context);
    return;
  }
  else
  {
    
  }
}

void HttpWorker::on_set_user_tag(uint64_t context_id,redisReply *reply)
{
  ContextIter iter=_context_map.find(context_id);
  if(iter==_context_map.end())
    return;
  boost::shared_ptr<InjectContext> context=iter->second;
  if(reply->type != REDIS_REPLY_INTEGER)
  {
    context->set_http_internal_err();
  }
  ++context->prog;
  schedule(context);
}

void HttpWorker::del_user_tags(boost::shared_ptr<InjectContext> context)
{
  LOG_INFO("del user tags,uid=%s,tag_no=%d",context->uid.c_str(),context->tag_no);
  redisAsyncContext * redis_context=_redis_client.get_redis_context(context->get_bucket_key());
  if(redis_context==NULL)
  {
    LOG_ERROR("%s get redis context error",context->uid.c_str());
    context->set_http_internal_err();
    http_done(context);
    return;
  }

  util::RedisOnReply *on_reply=
  util::RedisOnReply::new_instance(boost::bind(&HttpWorker::on_del_user_tags,this,context->id,_1));
  if(redisAsyncCommand(redis_context, util::redis_command_cb, (void *)on_reply,
        "hdel %s %s",context->get_bucket_key().c_str(),context->get_bucket_index().c_str())!=0)
  {
    LOG_ERROR("%s redis command error",context->uid.c_str());
    context->set_http_internal_err();
    http_done(context);
    return;
  }
  else
  {
    
  }
}

void HttpWorker::on_del_user_tags(uint64_t context_id,redisReply *reply)
{
  ContextIter iter=_context_map.find(context_id);
  if(iter==_context_map.end())
    return;
  boost::shared_ptr<InjectContext> context=iter->second;
  LOG_INFO("on del user tags,uid=%s,tag_no=%d",context->uid.c_str(),context->tag_no);
  if(reply->type != REDIS_REPLY_INTEGER)
  {
    context->set_http_internal_err();
  }
  context->prog=2;
  schedule(context);
}

}
}