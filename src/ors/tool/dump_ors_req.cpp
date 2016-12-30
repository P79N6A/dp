/**
 **/

//include STD C head files
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>

//include STD C++ head files


//include third_party_lib head files
#include "src/comm_event/comm_event.h"
#include "src/comm_event/comm_event_interface.h"
#include "src/comm_event/comm_event_factory.h"
#include "poseidon_proto.h"

using namespace dc::common::comm_event;
using namespace poseidon::ors;

int REQCOUNT=10000;
int cnt = 0;

class DumpOrsReqServer:public CommBase
{

public:
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr)
    {
        if (cnt > REQCOUNT)
        {
            exit(0);
        }
        AlgoRequest req;
        req.ParseFromArray(buf, len);

        char filename[16];
        snprintf(filename, 16, "reqs/req-%d", cnt++);
        std::ofstream ofs(filename);

        ofs << req.DebugString();
        ofs.close();
        return 0;
    }
};


int main(int argc, char * argv [])
{
    if(CommFactoryInterface::instance().init() != EC_SUCCESS)
    {
        printf("CommFactoryInterface::instance().init() return error\n");
        return -1;
    }
   
    REQCOUNT = 100;
    const char * pip="10.32.54.140";
    int port=10620;
    if (argc >= 2)
    {
        REQCOUNT = atoi(argv[1]);
    }
    if(argc >=  3)
    {
        pip=argv[2];
    }
    
    if(argc >= 4)
    {
        port=atoi(argv[3]);
    }

    printf("./dump_ors_req cnt=%d ip=%s port=%d", REQCOUNT, pip, port);
        
    CommBase * pUdp=new DumpOrsReqServer();
    pUdp->bindaddr(pip, port);
    CommFactoryInterface::instance().add_comm(pUdp);
    CommFactoryInterface::instance().run();
    delete pUdp;
}
