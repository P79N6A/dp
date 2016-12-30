#include "injector_inc.h"
#include "dmp_tag_id_map.h"
#include "task_scheduler.h"
#include "local_redis_map.h"
#include "redis_client.h"

DECLARE_bool(enable_redis_cluster);

int main(int argc, char** argv)
{
  google::SetVersionString("1.1.1");
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  
  if(!poseidon::TagIdMap::get_mutable_instance().init())
    return -1;
    
  int start_time=time((time_t)NULL);
  poseidon::TaskScheduler::get_mutable_instance().init();
  int loop=0;
  while(1)
  {
    ::sleep(1);
    loop++;
    if(loop%60==0)
      cout<<"dmp.injector running,unfinished task is "<<poseidon::TaskScheduler::get_mutable_instance().get_unfinished_task()<<endl;
    if(poseidon::TaskScheduler::get_mutable_instance().finished_all_task())
    {
      cout<<"all parse tasks done..."<<endl;
      int user_data_num=poseidon::LocalRedisMap::get_mutable_instance().get_user_data_num();
      if(user_data_num!=0)
      {
        cout<<"waiting for pack process,user data num="<<user_data_num<<endl;
        poseidon::LocalRedisMap::get_mutable_instance().package(true);
      }
      else
      {
        int pack_task_num=poseidon::LocalRedisMap::get_mutable_instance().get_pack_task_num();
        if(pack_task_num!=0)
        {
          cout<<"waiting for inject redis,task num="<<pack_task_num<<endl;
        }
        else
        {
          int end_time=time((time_t)NULL);
          cout<<"done!!!use time : "<<end_time-start_time<<endl;
          exit(0);
        }
      }
    }
  }
}