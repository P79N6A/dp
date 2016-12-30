#pragma once

#include "injector_inc.h"
namespace poseidon
{  

  struct TagData
  {
    uint32_t tag_no;
    vector<uint32_t> values;
  };
  struct LocalUserData
  {
    string uid;
    vector<boost::shared_ptr<TagData> >  datas;
    util::Mutex mutex;
  };
  
  
  typedef boost::unordered_map<string,boost::shared_ptr<LocalUserData> >::iterator UserDataIter;
  
  class LocalRedisMap:public boost::serialization::singleton<LocalRedisMap>
  { 
    public:
      LocalRedisMap()
      {
        _user_data_num=0;
        _packing_task_num=0;
        _adding_task=false;
      }
      ~LocalRedisMap()
      {
        
      }
      uint32_t get_user_data_num()
      {
        return _user_data_num;
      }
      uint32_t get_pack_task_num()
      {
        return _packing_task_num;
      }

      bool insert(const string &uid,uint32_t tag_no,uint32_t value);
      bool insert(const string &uid,uint32_t tag_no,const vector<uint32_t> &values);
      void package(bool force);
      void print_new_uid_info();
    
    protected:
      boost::unordered_map<string,boost::shared_ptr<LocalUserData> > _user_data_map;
      util::Mutex _user_data_map_mutex;
      bool _packing;
      util::Mutex _pack_mutex;
      boost::atomic<uint32_t> _user_data_num;
      boost::atomic<uint32_t> _packing_task_num;
      bool _adding_task;
    
    protected:
      void get_user_data(const string &uid,boost::shared_ptr<LocalUserData> &user_data);
      void done_pack_task();
      void package_process(vector<boost::shared_ptr<LocalUserData> > user_datas);
  };
};