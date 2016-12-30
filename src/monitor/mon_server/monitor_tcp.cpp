/**
 **/

#include <sstream> 
#include <algorithm> 

#include "monitor_tcp.h"
#include "util/log.h"
#include "protocol/src/poseidon_proto.h"
#include "monitor_mysql.h"
#include "monitor_tans.h"


namespace poseidon
{
namespace monitor
{

int MonitorConn::handle_pkg(const char * buf, const int len)
{
    int rt=0;
    do{
        MonitorTans::get_mutable_instance().trans(buf, len);
        monitor::ReportInfoReq req;
        if(!req.ParseFromArray(buf, len))
        {
            LOG_ERROR("Parse error, len[%d]", len);
            rt=-1;
            break;
        }
        LOG_DEBUG("recv[%s]\n", req.DebugString().c_str());
        rt=report2db(req);
        if(rt != 0)
        {
            LOG_ERROR("report2db error[%d]\n", rt);
            rt=-1;
            break;
        }

    }while(0);
    return rt;
}

int MonitorConn::on_error()
{
    LOG_DEBUG("MonitorConn on_error");
    delete this;
    return 0;
}

int MonitorConn::on_close()
{
    LOG_DEBUG("MonitorConn on_error");
    close_conn();
    delete this;
    return 0;
}

int MonitorConn::report2db(ReportInfoReq & req)
{
    int rt=0;
    do{
        int nrt=0;
//        int64_t seq=req.seq();
        int time_min=req.time_minute();
        std::string hostname=req.hostname();
        std::string hostip=req.hostip();
        int size_attr=req.attrs_size();
//        std::map<int, int64_t> map_attr;

        //mysql database-name不支持“-”， 替换成"_"
        std::replace(hostname.begin(), hostname.end(), '-', '_');

        //创建数据库
        std::stringstream ss;
        ss<<"create database if not exists "<<hostname;
        LOG_DEBUG("exec[%s]", ss.str().c_str());
        nrt=MonitorMysql::get_mutable_instance().exec(ss.str().c_str());
        if(nrt != 0)
        {
            LOG_ERROR("MonitorMysql::exec[%s]error[%d]\n",
                    ss.str().c_str(), nrt);
        }

        //插入数据
        for(int i=0; i < size_attr; i++)
        {
            const AttrInfo & attrinfo =req.attrs(i);
            int attr_id=attrinfo.attr_id();
            int64_t val=attrinfo.value();

            //创建数据库表
            std::stringstream ss1;
            ss1<<"create table if not exists "<<hostname<<".attr_"<<attr_id<<" like monitor_config.attr_data_temp";
            LOG_DEBUG("exec[%s]", ss1.str().c_str());
            nrt=MonitorMysql::get_mutable_instance().exec(ss1.str().c_str());
            if(nrt != 0)
            {
                LOG_ERROR("MonitorMysql::exec[%s]error[%d]\n",
                        ss.str().c_str(), nrt);
            }

//            map_attr[attrinfo.attr_id()]=attrinfo.value();
//insert into attr_3 set attr_id=3,time_min=123456,value=100;
            std::stringstream ss;
            ss<<"insert into "<<hostname<<".attr_"<<attr_id;
            ss<<" set attr_id="<<attr_id<<",time_min="<<time_min;
            ss<<",value="<<val;
            LOG_DEBUG("exec[%s]", ss.str().c_str());
            nrt=MonitorMysql::get_mutable_instance().exec(ss.str().c_str());
            if(nrt != 0)
            {
                LOG_ERROR("MonitorMysql::exec[%s]error[%d]\n",
                        ss.str().c_str(), nrt);
            }
        }

    }while(0);
    return rt;
}

}
}

