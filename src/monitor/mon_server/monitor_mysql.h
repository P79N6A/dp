/**
 **/


#ifndef  _MONITOR_MYSQL_H_ 
#define  _MONITOR_MYSQL_H_

#include <boost/serialization/singleton.hpp>
#include "protocol/src/poseidon_proto.h"
#include "mysql_public_iface.h"

namespace poseidon
{
namespace monitor
{

class MonitorMysql:public boost::serialization::singleton<MonitorMysql>
{
public:
    MonitorMysql():driver_(NULL),conn_(NULL),stmt_(NULL)
    {
    }

    /**
     * @brief               初始化
     **/
    int init(const char * host, const char * user, const char * passwd);


    /**
     * @brief               执行命令行
     * @param cmdline       [IN], 执行的命令
     * @return              success return 0, or return other
     **/
    int exec(const char * cmdline);

private:
    sql::Driver * driver_;
    sql::Connection * conn_;
    sql::Statement * stmt_;

};

}//monitor
}//poseidon

#endif   // ----- #ifndef _MONITOR_MYSQL_H_  ----- 


