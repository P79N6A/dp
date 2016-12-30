/**
 **/
#include <sys/socket.h>
#include <sys/types.h>
#include "util/func.h"
#include <iostream>
#include <fstream>
#include <string>
#include "src/ha/ha.h"
#include "protocol/src/poseidon_proto.h"
#include "util/proto_helper.h"
using namespace poseidon;

#define LOG_ERROR(fmt, a...) fprintf(stderr, fmt, ##a )
void pack(char * buf, int & buflen)
{
    poseidon::rtb::BidRequest req;
    util::ParseProtoFromTextFormatFile("req", &req);
    std::string str;
    req.SerializeToString(&str);
    printf("req[%s]\n", req.DebugString().c_str());
    buflen=str.length();
    memcpy(buf, str.c_str(), buflen);
}

int main(int argc, char * argv[])
{
    int rt=0;
    int sock=-1;
    do{
        if(argc < 2)
        {
            LOG_ERROR("usage:%s cnt", argv[0]);
            rt=-1;
            break;
        }
        sock=socket(AF_INET, SOCK_DGRAM, 0);
        if(sock < 0)
        {
            break;
        }
        HA_INIT("10.32.50.180:2181,10.32.50.181:2181,10.32.50.182:2181,10.32.50.212:2181,10.32.50.213:2181");
        uint64_t cnt=strtoul(argv[1], NULL, 0);
        char buf[4096];
        int array[100];
        memset(array, 0x0, sizeof(array));
        for(uint64_t i=0; i<cnt; i++)
        {
            int buflen=4096;
            memset(buf, 0x00, 4096);
            pack(buf, buflen);
            struct sockaddr_in addr;
            HA_GET_ADDR("controler", addr);
            uint64_t t1,t2;
            poseidon::util::Func::get_time_ms(t1);
            sendto(sock, buf, buflen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
            recvfrom(sock, buf, 4096, 0, NULL, NULL);
            poseidon::util::Func::get_time_ms(t2);
            int ms=(t2-t1>=100)?99:(t2-t1);
            array[ms]++;
        }
        for(int i=0; i<100; i++)
        {
            if(array[i] > 0)
            {
                printf("ms[%d]=%d\n", i, array[i] );
            }
        }

    }while(0);
    if(sock > 0)
    {
        close(sock);
    }
    return rt;
}

