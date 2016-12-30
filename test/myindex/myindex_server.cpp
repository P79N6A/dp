/**
 **/

//include STD C head files
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

//include STD C++ head files


//include third_party_lib head files
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "poseidon_proto.h"
#include "myindex.h"
#include "myindex.pb.h"

using namespace dc::common::comm_event;
using namespace poseidon::rtb;
namespace pb_mi=poseidon::myindex;

#define LOG_ERROR(fmt, a...) fprintf(stderr, fmt, ##a);

static MyIndex & MIinstance()
{
    static MyIndex mi;
    return mi;
}

class MyUdpServer:public CommBase
{

public:
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
    {
        int rt=0;
        do{
            pb_mi::MiRequest req;
            pb_mi::MiResponse resp;
            req.ParseFromArray(buf, len);
//            printf("recv[%s]\n", req.DebugString().c_str());
         
            std::vector<int> vrtarget;
            std::set< ::Ad > setad;
         
            int target_size=req.targeting_size();
            for(int i=0; i< target_size; i++)
            {
                vrtarget.push_back(req.targeting(i));
            }
            rt=MIinstance().query(vrtarget, setad );
            if(rt != 0)
            {
                LOG_ERROR("MIinstance query return error[%d]\n", rt);
                break;
            }
            std::set<Ad>::iterator it;
            for(it=setad.begin(); it != setad.end(); it++)
            {
                pb_mi::Ad * pAd=resp.add_ads();
                pAd->set_name(it->name);
            }
            std::string send_str;         
            if(!resp.SerializeToString(&send_str))
            {
                printf("recv[%s]\n", req.DebugString().c_str());
            }
//            printf("resp[%s]\n", resp.DebugString().c_str());
            send_pkg(send_str.c_str(), send_str.length(), client_addr );
        }while(0);
        return rt;
    }
};

int init_net(int argc, char * argv[])
{
    int rt=0;
    do{
        if(CommFactoryInterface::instance().init() != EC_SUCCESS)
        {
            printf("CommFactoryInterface::instance().init() return error\n");
            rt=-1;
            break;
        }
        const char * pip="0.0.0.0";
        int port=1234;
        if(argc >=  2)
        {
            pip=argv[1];
        }
        if(argc >= 3)
        {
            port=atoi(argv[2]);
        }
        
        CommBase * pUdp=new MyUdpServer();
        pUdp->bindaddr(pip, port);
        CommFactoryInterface::instance().add_comm(pUdp);
    }while(0);
    return rt;
}
int run()
{
    CommFactoryInterface::instance().run();
    return 0;
}

int build_index_graph()
{
    int rt=0;
    do{
        rt=MIinstance().parse_from_file("./ad.data");
        if(rt != 0)
        {
            break;
        }
    }while(0);
    return rt;
}



int main(int argc, char * argv [])
{
    int rt=0;
    do{
        rt=init_net(argc, argv);
        if(rt != 0)
        {
            LOG_ERROR("init_net return error[%d]\n", rt);
            break;
        }
        rt=build_index_graph();
        if(rt !=0)
        {
            LOG_ERROR("build_index_graph return error[%d]\n", rt);
            break;
        }
        rt=run();
        if(rt != 0)
        {
            LOG_ERROR("run return error[%d]\n", rt);
            break;
        }
    }while(0);
    return rt;

}



