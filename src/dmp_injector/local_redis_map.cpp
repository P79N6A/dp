#include "local_redis_map.h"
#include "task_scheduler.h"
#include "comm_fun.h"
#include "redis_client.h"

DEFINE_int32(max_user_data_num,1000000,"max_user_data_num");
DEFINE_bool(enable_inject_redis,true,"enable_inject_redis");

namespace poseidon
{
  
  void LocalRedisMap::package_process(vector<boost::shared_ptr<LocalUserData> > user_datas)
  {
    while(FLAGS_enable_inject_redis)
    {
      RedisClient redis_client;
      if(!redis_client.init())
      {
        cout<<"redis client init error..."<<endl;
        break;
      }

      usleep(500*1000);
      for(int i=0;i<user_datas.size();i++)
      {
        boost::shared_ptr<LocalUserData> lud=user_datas[i];
        string uid=lud->uid;
        
        string bucket_key=get_bucket_key(uid);
        string bucket_index=get_bucket_index(uid);
        redisContext * redis_context=redis_client.get_redis_context(bucket_key);
        redisReply* r1=(redisReply*)redisCommand(redis_context, "hget %s %s",bucket_key.c_str(),bucket_index.c_str());

        ScopeReply sr1(r1);
        if (NULL == r1) 
        {  
          cout<<uid<<",redis get error,reply is null"<<endl;
          break;
        } 
        
        map<uint32_t,uint32_t> tag_index_map;
        dmp::DmpUserData dmp_user_data;
        string source_md5_str;
        if (r1->type == REDIS_REPLY_NIL)
        {  

        }
        else if(r1->type == REDIS_REPLY_STRING)
        {
          dmp_user_data.ParseFromArray(r1->str,r1->len);
          util::Func::md5sum(r1->str,r1->len,source_md5_str);
          for(int j=0;j<dmp_user_data.tag_datas_size();j++)
          {
            tag_index_map[dmp_user_data.tag_datas(j).tag_no()]=j;
          }
        }
        else
        {
          cout<<uid<<",redis get error;type="<<r1->type<<" str="<<r1->str<<endl;
          break;
        }
        
        for(int j=0;j<lud->datas.size();j++)
        {
          map<uint32_t,uint32_t>::iterator iter=tag_index_map.find(lud->datas[j]->tag_no);
          if(iter==tag_index_map.end())
          {
            dmp::DmpUserData::TagData * tag_data=dmp_user_data.add_tag_datas();
            tag_data->set_tag_no(lud->datas[j]->tag_no);
            for(int k=0;k<lud->datas[j]->values.size();k++)
              tag_data->add_values(lud->datas[j]->values[k]);
          }
          else
          {
            dmp::DmpUserData::TagData * tag_data=dmp_user_data.mutable_tag_datas(iter->second);
            tag_data->Clear();
            tag_data->set_tag_no(lud->datas[j]->tag_no);
            for(int k=0;k<lud->datas[j]->values.size();k++)
              tag_data->add_values(lud->datas[j]->values[k]);
          }
        }
        
        char buffer[1024*128]={0};
        if(dmp_user_data.ByteSize()>sizeof(buffer))
        {
          cout<<"buffer too smal"<<endl;
          break;
        }
        dmp_user_data.SerializeToArray(buffer,sizeof(buffer));
        
        string cur_md5_str;
        util::Func::md5sum(buffer,dmp_user_data.ByteSize(),cur_md5_str);
        if(source_md5_str.compare(cur_md5_str)!=0)
        {
          //cout<<"uid="<<uid<<"`source_md5="<<source_md5_str<<endl;
          //cout<<"uid="<<uid<<"`cur_md5="<<cur_md5_str<<endl;
        
          redisReply* r2=(redisReply*)redisCommand(redis_context, "hset %s %s %b",
                    bucket_key.c_str(),bucket_index.c_str(),buffer,dmp_user_data.ByteSize());
          ScopeReply sr2(r2);
          if (NULL == r2) 
          {  
            cout<<uid<<",redis hset error,reply is null"<<endl;
            break;
          } 
          if(r2->type != REDIS_REPLY_INTEGER)
          {
            cout<<uid<<",redis hset error"<<endl;
            break;
          }
          cout<<"uid "<<uid<<" set succ"<<endl;
        }
      }
    
      break;
    }
    done_pack_task();
  }
  
  void LocalRedisMap::done_pack_task()
  {
    _packing_task_num--;
    if(_packing_task_num==0)
    {
      while(_adding_task)
      {
        sleep(1);
      }
      if(_packing)
      {
        _packing=false;
      }
    }
  }
  
  
  bool LocalRedisMap::insert(const string &uid,uint32_t tag_no,const vector<uint32_t> &values)
  {
    if(_user_data_num>FLAGS_max_user_data_num*2)
    {
      package(false);
      return false;
    }
    boost::shared_ptr<LocalUserData> lud;
    get_user_data(uid,lud);
    {
      util::ScopeWMutex sw_mutex(&(lud->mutex));
      int pos=0;
      for(;pos<lud->datas.size();pos++)
      {
        if(tag_no==lud->datas[pos]->tag_no)
          break;
      }
      if(pos<lud->datas.size())
      {
        lud->datas[pos]->values=values;
      }
      else
      {
        boost::shared_ptr<TagData> td(new TagData);
        td->tag_no=tag_no;
        td->values=values;
        lud->datas.push_back(td);
      }
    }
    package(false);
    return true;
  }
  
  bool LocalRedisMap::insert(const string &uid,uint32_t tag_no,uint32_t value)
  {
    if(_user_data_num>FLAGS_max_user_data_num*2)
    {
      package(false);
      return false;
    }
    boost::shared_ptr<LocalUserData> lud;
    get_user_data(uid,lud);
    {
      util::ScopeWMutex sw_mutex(&(lud->mutex));
      int pos=0;
      for(;pos<lud->datas.size();pos++)
      {
        if(tag_no==lud->datas[pos]->tag_no)
          break;
      }
      if(pos<lud->datas.size())
      {
        lud->datas[pos]->values.push_back(value);
      }
      else
      {
        boost::shared_ptr<TagData> td(new TagData);
        td->tag_no=tag_no;
        td->values.push_back(value);
        lud->datas.push_back(td);
      }
    }
    package(false);
    return true;
  }
  
  void LocalRedisMap::get_user_data(const string &uid,boost::shared_ptr<LocalUserData> &user_data)
  {
    bool insert=false;
    _user_data_map_mutex.wlock();
    UserDataIter iter=_user_data_map.find(uid);
    if(iter==_user_data_map.end())
    {
      insert=true;
    }
    else
    {
      user_data=iter->second;
    }
    if(insert)
    {
      boost::shared_ptr<LocalUserData> lud(new LocalUserData);
      user_data=lud;
      _user_data_map[uid]=lud;
      _user_data_num++;
    }
    _user_data_map_mutex.unlock();
  }
  
  void LocalRedisMap::package(bool force)
  {
    if(!force)
    {
      if(_user_data_num<FLAGS_max_user_data_num)
        return;
    }

    if(_packing)
      return;
    {
      util::ScopeWMutex sw_mutex(&_pack_mutex);
      if(_packing)
        return;
      _packing=true;
    }
    
    cout<<"packing...."<<endl;
    _user_data_map_mutex.wlock();
    boost::unordered_map<string,boost::shared_ptr<LocalUserData> > user_data_map=_user_data_map;
    _user_data_map.clear();
    _user_data_num=0;
    _user_data_map_mutex.unlock();
    UserDataIter data_iter;
    vector<boost::shared_ptr<LocalUserData> > user_data_vec;
    _adding_task=true;
    for(data_iter=user_data_map.begin();data_iter!=user_data_map.end();data_iter++)
    {
      data_iter->second->uid=data_iter->first;
      user_data_vec.push_back(data_iter->second);
      if(user_data_vec.size()>FLAGS_max_user_data_num/10)
      {
        _packing_task_num++;
        util::Closure *closure=util::NewCallback(this,&LocalRedisMap::package_process,user_data_vec);
        util::ThreadPool::get_mutable_instance().add_task(closure);
        user_data_vec.clear();
      }
    }
    if(user_data_vec.size()>0)
    {
      _packing_task_num++;
      util::Closure *closure=util::NewCallback(this,&LocalRedisMap::package_process,user_data_vec);
      util::ThreadPool::get_mutable_instance().add_task(closure);
    }
    _adding_task=false;
  }
}