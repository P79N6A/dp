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
using namespace dc::common::comm_event;
using namespace poseidon::rtb;

class MyUdpServer:public CommBase
{

public:
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
    {
        BidRequest req;
        BidResponse resp;
        req.ParseFromArray(buf, len);
        printf("recv[%s]\n", req.DebugString().c_str());
        resp.set_id(req.id());
        resp.set_no_bid_reason(1);
        char send_buf[10000];
        int buflen=10000;
        if(!resp.SerializeToArray(send_buf, buflen))
        {
            printf("recv[%s]\n", req.DebugString().c_str());
        }
        send_pkg(send_buf, buflen, client_addr );
        return 0;
    }
};

int main(int argc, char * argv [])
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
        CommFactoryInterface::instance().run();
        delete pUdp;
    }while(0);
    return rt;

}
