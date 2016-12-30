/**
 **/

#ifndef  _PROCESS_LOG_H_ 
#define  _PROCESS_LOG_H_

#include <stdio.h>
#include <iostream>

#include <boost/serialization/singleton.hpp>

namespace poseidon
{
namespace log
{

class ProcessLog:public boost::serialization::singleton<ProcessLog>
{
public:
    ProcessLog():fp_(NULL),last_rolling_time_(-1),buf_idx_(0){}

    /**
     * @brief           初始化
     **/
    int init();

    /**
     * @brief           处理日志
     **/
    int proc(const char * buf, int buflen);

    /**
     * @brief           把数据push到文件 
     */
    int push_to_file(const char * buf, int buflen);

    /**
     * @brief           roll file
     **/    
    int roll_file();

private:
    FILE * fp_;                 //写文件的句柄
    int rolling_time_;          //多久rolling文件
    int last_rolling_time_;     //上一次rolling的时间
    std::string path_;          //文件路径
    std::string file_prefix_;   //文件名前缀
    std::string current_file_;  //当前正在写的文件.tmp
    std::string current_dest_file_; //当前目标文件.log
    int buf_idx_;               //buf里的index，标识下次写的位置
    int proc_idx_;              //进程index
    std::string str_proc_idx_;  //字符串型大的进程index
    char buf_[4096];            //cache日志
};

}
}

#endif   // ----- #ifndef _PROCESS_LOG_H_  ----- 
