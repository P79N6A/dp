/**
 **/
#include <sys/socket.h>
#include <sys/types.h>
#include "util/func.h"
#include <iostream>
#include <string>
#include "ha.h"
#include "protocol/src/poseidon_proto.h"
#include "util/proto_helper.h"
using namespace poseidon;

#define LOG_ERR(fmt, a...) fprintf(stderr, fmt, ##a )

void pack(char * buf, int & buflen)
{
    qp::QPRequest req;
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
        if(argc < 3)
        {
            LOG_ERR("usage:%s ip port", argv[0]);
            rt=-1;
            break;
        }
        sock=socket(AF_INET, SOCK_DGRAM, 0);
        if(sock < 0)
        {
            break;
        }
        const char * ip=argv[1];
        int port=atoi(argv[2]);

        char buf[4096];
        int buflen=4096;
        memset(buf, 0x00, 4096);
        pack(buf, buflen);
        struct sockaddr_in addr;
        MakeAddr(addr, ip, port);
        sendto(sock, buf, buflen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
        int nrecv=recvfrom(sock, buf, 4096, 0, NULL, NULL);
        if(nrecv < 0)
        {
            LOG_ERR("recvfrom error[%s]\n", strerror(errno));
            break;
        }
        qp::QPResponse rsp;
        if(!rsp.ParseFromArray(buf, nrecv))
        {
            LOG_ERR("rsp.ParseFromArray error");
            break;
        }
        printf("rsp[%s]\n", rsp.DebugString().c_str());

    }while(0);
    if(sock > 0)
    {
        close(sock);
    }
    return rt;
}

