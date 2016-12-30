/**
 **/

#include "agent_proc.h"
#include "util/shm.h"
#include "util/log.h"
#include "util/func.h"
#include "config.h"
#include "protocol/src/poseidon_proto.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

namespace poseidon
{
namespace monitor
{

int AgentProc::init()
{
    int rt=0;
    do{
        /*创建共享内存*/
        layout_=(MonitorLayOut *)util::shm::ShmAttach(MONITOR_KEY, sizeof(MonitorLayOut), 0666);
        if(layout_ == NULL)
        {
            layout_=(MonitorLayOut *)util::shm::ShmCreate(MONITOR_KEY, sizeof(MonitorLayOut), 0666);
            if(layout_ == NULL)
            {
                rt =-1;
                break;
            }
            //共享内存初始化
            layout_->head_.max_attr_id_=0;
        }
        MakeAddr(serv_addr_, Config::get_mutable_instance().server_ip(), Config::get_mutable_instance().server_port());

        
    }while(0);
    return rt;
}

int AgentProc::run()
{
    int rt=0;
    do{
        if(layout_ == NULL)
        {
            rt=-1;
            break;
        }
        do{
            time_t now = time(NULL);
            int sleep_time=60-(now%60);
            sleep(sleep_time);//sleep到下一个时间整点

            int time_min=now/60;

            ReportInfoReq reportinfo;
            rt=collect_data(time_min, reportinfo);
            if(rt != 0)
            {
                LOG_ERROR("collect_data return error[%d]\n", rt);
                continue;
            }
            report(reportinfo);
        }while(1);
    }while(0);
    return rt;
}
int AgentProc::report(ReportInfoReq & reportinfo)
{
    int rt=0;
    do{
        std::string sendbuf; 
        if(!reportinfo.SerializeToString(&sendbuf))
        {
            LOG_ERROR("reportinfo SerializeToArray error");
            rt=-1;
            break;
        }
        LOG_DEBUG("report[%s]\n", reportinfo.DebugString().c_str());
        rt=send_report(sendbuf);
        if(rt != 0)
        {
            LOG_ERROR("send_report return error;");
            rt=-1;
            break;
        }

    }while(0);
    return rt;
}
int AgentProc::send_report(std::string &sendbuf)
{
    int rt=0;
    do{
        if(sock_ < 0)
        {
            rt=connect_server();
            if(rt != 0)
            {
                LOG_ERROR("connect_server error[%d]\n", rt);
                break;
            }
        }
        int nhassend=0;
        int nsend=sendbuf.size();
        while(nhassend < nsend)
        {
            int nthislen=nsend-nhassend; 
            int nrt=send(sock_, sendbuf.c_str()+nhassend, nthislen, 0);
            if(nrt == -1)
            {
                if(errno == EPIPE   ||
                   errno == ECONNRESET ||
                   errno == ENOTCONN ||
                   errno== EINTR )
                {
                    rt=connect_server();
                    if(rt != 0)
                    {
                        LOG_ERROR("connect_server error[%d]\n", rt);
                        break;
                    }
                }else
                {
                    LOG_ERROR("connect_server error[%s]\n",
                        strerror(errno) );
                    break;
                }
            }else
            {
                nhassend+=nrt;
            }
        }
    }while(0);
    return rt;
}

int AgentProc::connect_server()
{
    int rt=0;
    do{
        sock_=socket(AF_INET, SOCK_STREAM, 0);
        if(sock_ < 0)
        {
            LOG_ERROR("socket error[%s]\n", strerror(errno));
            rt=-1;
            break;
        }

        rt=connect(sock_, (struct sockaddr *)&serv_addr_, sizeof(struct sockaddr_in) );
        if(rt < 0)
        {
            LOG_ERROR("connect error[%s]\n", strerror(errno));
            rt=-2;
            break;
        }
        
        
    }while(0);
    return rt;

}

int AgentProc::collect_data(int time_min, ReportInfoReq & reportinfo)
{
    int rt=0;
    do{
        int max_attr_id=layout_->head_.max_attr_id_;
        reportinfo.set_seq(time_min);
        char hostname[256];
        gethostname(hostname, 256);
        reportinfo.set_hostname(hostname);
        reportinfo.set_hostip(Config::get_mutable_instance().local_ip());
        reportinfo.set_time_minute(time_min);
        MonitorBody & body=layout_->body_;
        
        for(int i=0; i<=max_attr_id; i++)
        {
            monitor::Attr & attr=body.attr_[i];
//            bool valid=true;
            if(body.attr_[i].usedflag_ == USED_FLAG)
            {
                //更新index_
                int old_index=attr.index_;
                int new_index=(attr.index_+1)%RESERVE_MIN_CNT;
                attr.data_[new_index].min_=time_min+1;
                attr.data_[new_index].value_=0;
                attr.index_=new_index;
                usleep(1);//留个很小的时间窗,让获得老的Index的api,可以写到老的data里面去
                if(time_min==attr.data_[old_index].min_)
                {
                    int64_t value=attr.data_[old_index].value_;
                    monitor::AttrInfo * pattr=reportinfo.add_attrs();
                    pattr->set_attr_id(i);
                    pattr->set_value(value);
                }

#if 0
//                int real_idx=0;
                int idx=attr.index_;
                if(idx>=RESERVE_MIN_CNT)
                {
                    continue;
                }
                while(1)
                {
                    if( attr.data_[idx].min_ == time_min )
                    {
                        value=attr.data_[idx].value_;
                        break;
                    }else if(attr.data_[idx].min_ < time_min)
                    {
                        valid=false;
                        break;
                    }
                    idx=(idx+RESERVE_MIN_CNT-1)%RESERVE_MIN_CNT;
                    if(idx==attr.index_)
                    {
                        /*转一圈回来了*/
                        valid=false;
                        break;
                    }
                }
                if(!valid)
                {
                    continue;
                }
#endif

            }

            
        }
    }while(0);
    return rt;
}

}//monitor

}//poseidon

