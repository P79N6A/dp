/**
 **/

#include "process_log.h"

#include <errno.h>

#include "config.h"
#include "monitor_api.h"
#include "util/log.h"
#include "util/func.h"
#include "log_server_attr.h"

namespace poseidon
{
namespace log
{


/**
 * @brief           初始化
 **/
int ProcessLog::init()
{
    int rt=0;
    do{
        path_=Config::get_mutable_instance().get_path();
        file_prefix_=Config::get_mutable_instance().get_file_prefix();
        rolling_time_=Config::get_mutable_instance().get_rolling_time();
        proc_idx_=Config::get_mutable_instance().process_idx();
        str_proc_idx_=util::Func::to_str(proc_idx_);
        rt=roll_file();
        if(rt != 0)
        {
            LOG_ERROR("roll file error[%d]", rt);
            break;
        }

    }while(0);
    return rt;
}

/**
 * @brief           处理日志
 **/
int ProcessLog::proc(const char * buf, int buflen)
{
    int rt=0;
    do{
        if(buflen > 2048)
        {
            //直接写文件，不做cache
            push_to_file(buf, buflen);
            break;
        }

        if(buf_idx_ + buflen > 4096 )
        {
            push_to_file(buf_, buf_idx_);
            buf_idx_=0;
        }
        memcpy(buf_+buf_idx_, buf, buflen);
        buf_idx_+=buflen;
    }while(0);
    return rt;
}

/**
 * @brief           把数据push到文件 
 */
int ProcessLog::push_to_file(const char * buf, int buflen)
{
    int rt=0;
    do{
        MON_ADD(ATTR_PUSH_TO_FILE, 1);
        time_t now=time(NULL);
        if(now - last_rolling_time_ >= rolling_time_ )
        {
            rt=roll_file();
            if(rt != 0)
            {
                LOG_ERROR("roll file error[%d]", rt);
                break;
            }
        }
        if(fp_==NULL)
        {
            fp_=fopen(current_file_.c_str(), "w+");
            if(fp_ == NULL)
            {
                LOG_ERROR("fopen error[%s]\n", current_file_.c_str());
                rt=-1;
                break;
            }
        }
        int nhaswrite=0;
        int nwrite=0;
        while(1)
        {
            nwrite=fwrite(buf+nhaswrite, 1, buflen-nhaswrite, fp_);
            if(nwrite <= 0)
            {
                MON_ADD(ATTR_FWRITE_ERR, 1);
                LOG_ERROR("fwrite error");
                break;
            }
            nhaswrite+=nwrite;
            if(nhaswrite >= buflen)
            {
                break;
            }
        }

    }while(0);
    return rt;
}

int ProcessLog::roll_file()
{
    int rt=0;
    do{
        if(fp_ != NULL)
        {
            fclose(fp_);
            fp_=NULL;
            if(rename(current_file_.c_str(), current_dest_file_.c_str())<0)
            {
                LOG_ERROR("rename(%s, %s) return error[%s]", current_file_.c_str(), current_dest_file_.c_str(), strerror(errno));
                rt=-1;
                break;
            }
        }
        std::string strtime;
        util::Func::get_time_str(&strtime, "%Y-%m-%d-%H-%M-%S");
        current_file_=path_+"/"+file_prefix_+"_"+str_proc_idx_+"_"+strtime+".tmp";
        current_dest_file_=path_+"/"+file_prefix_+"_"+str_proc_idx_+"_"+strtime+".log";
        fp_=fopen(current_file_.c_str(), "w+");
        if(fp_ == NULL)
        {
            LOG_ERROR("fopen return error");
            rt=-1;
            break;
        }
        last_rolling_time_=time(NULL);
        
    }while(0);
    return rt;
}


}//log
}//poseidon

