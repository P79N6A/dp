/**
 **/


#include "monitor_mysql.h"
#include "util/log.h"

namespace poseidon
{
namespace monitor
{

/**
 * @brief               初始化
 **/
int MonitorMysql::init(const char * host, const char * user, const char * passwd)
{
    int rt=0;
    do{
        try{
            driver_=sql::mysql::get_driver_instance();
            conn_=driver_->connect(host, user, passwd);
            stmt_=conn_->createStatement();
        }catch(sql::SQLException & e)
        {
            LOG_ERROR("init error[%s]", e.what());
            rt=-1;
        }
    }while(0);
    return rt;
}


/**
 * @brief               执行命令行
 * @param cmdline       [IN], 执行的命令
 * @return              success return 0, or return other
 **/
int MonitorMysql::exec(const char * cmdline)
{
    int rt=0;
    do{
        try{
            stmt_->execute(cmdline);
        }catch(sql::SQLException & e)
        {
            LOG_ERROR("exec[%s] error[%s]", cmdline, e.what());
            rt=-1;
        }
    }while(0);
    return rt;
}




}//monitor
}//poseidon

