#include "task_scheduler.h"
#include "dmp_tag_id_map.h"
#include "file_parse_task.h"
#include "dmp_device_task.h"
#include "comm_fun.h"
DEFINE_string(dmp_data_path, "./dmp.data", "dmp_data path");
DEFINE_bool(enable_device_process,true,"enable_device_process");
DEFINE_bool(enable_tag_process,true,"enable_tag_process");

namespace poseidon
{
  void thread_device_task_run()
  {
    if(FLAGS_enable_device_process)
      DeviceTask::get_mutable_instance().process();
  }
  
  void thread_task_run(queue<FileParseTaskInfo> task_info_queue)
  {

    while(1)
    {
      FileParseTaskInfo fpti;
      if(task_info_queue.size()>0)
      {
        fpti=task_info_queue.front();
        task_info_queue.pop();
        FileParseTask fpt(fpti.tag_no,fpti.value_type,fpti.file_path);
        fpt.parse();
        TaskScheduler::get_mutable_instance().done_task();
      }
      else
      {
        break;
      }
    }
  }
  void TaskScheduler::init()
  {
    util::ThreadPool::get_mutable_instance().add_task(boost::bind(thread_device_task_run));
    
    if(!FLAGS_enable_tag_process)
    {
      return;
    }
    vector<TagIdInfo> tag_id_infos;
    TagIdMap::get_mutable_instance().get_all(tag_id_infos);
    _task_num=0;
    for(int i=0;i<tag_id_infos.size();i++)
    {
      TagIdInfo info=tag_id_infos[i];
      string tag_data_path=FLAGS_dmp_data_path+"/"+info.tag_id;
      vector<string> file_names;
      if(get_all_filenames(tag_data_path,file_names)==0)
      {
        queue<FileParseTaskInfo> task_info_queue;
        for(int j=0;j<file_names.size();j++)
        {
          string tag_file_path=tag_data_path+"/"+file_names[j];
          FileParseTaskInfo fpti;
          fpti.file_path=tag_file_path;
          fpti.tag_no=info.tag_no;
          fpti.value_type=info.value_type;
          task_info_queue.push(fpti);
          _task_num++;
        }
        util::ThreadPool::get_mutable_instance().add_task(boost::bind(thread_task_run,task_info_queue));
      }
    }
  }
}