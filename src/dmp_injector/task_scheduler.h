#pragma once

#include "injector_inc.h"

namespace poseidon
{
  struct FileParseTaskInfo
  {
    string file_path;
    uint16_t tag_no;
    int value_type;
  };
  
  class TaskScheduler:public boost::serialization::singleton<TaskScheduler>
  {
    public:
      void init();
      bool get_task(FileParseTaskInfo & fpti);
      void done_task()
      {
        --_task_num;
      }
      int get_unfinished_task()
      {
        return _task_num;
      }
      bool finished_all_task()
      {
        if(_task_num<=0)
          return true;
        return false;
      }
    
    protected:
      boost::atomic<int32_t> _task_num;
      util::Mutex _task_queue_mutex;
      boost::threadpool::pool *_thread_pool;
      
    protected:
  };
}