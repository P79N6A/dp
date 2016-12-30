/**
 **/
#include <sys/socket.h>
#include <sys/types.h>
#include "util/func.h"
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include "protocol/src/poseidon_proto.h"
#include "util/proto_helper.h"
using namespace poseidon;

#define LOG_ERROR(fmt, a...) fprintf(stderr, fmt, ##a )
void pack(const char* reqfile, char * buf, int & buflen)
{
    ors::AlgoRequest req;
    util::ParseProtoFromTextFormatFile(reqfile, &req);
    std::string str;
    req.SerializeToString(&str);
    buflen=str.length();
    printf("%s,%d\n", reqfile, buflen);
    memcpy(buf, str.c_str(), buflen);
}

int main(int argc, char * argv[])
{
    int sock=-1;
    if (argc != 4)
    {
        printf("./test_client_ors ip port req_file\n");
        return -1;
    }

    const char* pip = argv[1];
    int port = atoi(argv[2]); 
    const char* reqfile = argv[3];

    sock=socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        return -1;
    }

    char buf[40960];
    int array[100];
    memset(array, 0x0, sizeof(array));
    int buflen=40960;
    memset(buf, 0x00, 40960);
    pack(reqfile, buf, buflen);
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family= AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(pip);
    uint64_t t1,t2;
    poseidon::util::Func::get_time_ms(t1);
    sendto(sock, buf, buflen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));

    char rbuf[4096];
    memset(buf, 0x00, sizeof(rbuf));
    int len = recvfrom(sock, rbuf, sizeof(rbuf), 0, NULL, NULL);
    ors::AlgoResponse response;
    response.ParseFromArray(rbuf, len);
    poseidon::util::Func::get_time_ms(t2);
    printf("reqfile=%s, usetime=%ld, error=%d, ad_num=%d\n", reqfile, t2-t1,response.error_code() ,response.algoed_ads_size());
    if(sock > 0)
    {
        close(sock);
    }
    return 0;
}

