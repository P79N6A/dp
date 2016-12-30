/**
 **/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "myindex.pb.h"
#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    
#include <iostream>    
#include <string>    
#include <errno.h>    
#include <sys/time.h>    

#define MakeAddr(toaddr, ip, port)  do{ \
    toaddr.sin_family=AF_INET;   \
    toaddr.sin_port=htons(port);    \
    inet_aton(ip, &toaddr.sin_addr);    \
}while(0)
#define LOG_DEBUG(fmt, a...) fprintf(stderr, "debug:"fmt, ##a)
#define LOG_ERROR(fmt, a...) fprintf(stderr, "error:"fmt, ##a)

using namespace poseidon::myindex;
using namespace std;

int main(int argc, char * argv [])
{
    int rt=0;
    int sock=-1;
    do{
        sock=socket(AF_INET, SOCK_DGRAM, 0);
        if(sock < 0)
        {
            rt=-1;
            break;
        }
        struct sockaddr_in toaddr;

        boost::property_tree::ptree m_pt;  
        string ini_file = "./myindex.ini";  
        boost::property_tree::ini_parser::read_ini(ini_file, m_pt);
        
        string ip= m_pt.get<string>("myindex.ip","");  
        int port = m_pt.get<int>("myindex.port", 1);

        MakeAddr(toaddr, ip.c_str(), port);

        char buf[1024];

        MiRequest req;
        MiResponse rsp;

        for(int i=1; i<argc; i++)
        {
            req.add_targeting(atoi(argv[i]));        
        }
        LOG_DEBUG("req[%s]\n", req.DebugString().c_str());
        std::string send_str;
        if(!req.SerializeToString(&send_str))
        {
            LOG_ERROR("req.SerializeToArray error\n");
            rt=-1;
            break;
        }
        timeval tv;
        gettimeofday(&tv, NULL);
        int nsendto=sendto(sock, send_str.c_str(), send_str.length(), 0,
                (struct sockaddr *)&toaddr, sizeof(struct sockaddr_in));
        if(nsendto < 0)
        {
            LOG_ERROR("sendto error[%s]\n", strerror(errno));
            rt=-1;
            break;
        }

        memset(buf, 0x00, 1024);
        int nrecv=recvfrom(sock, buf, 1024, 0, NULL, NULL);
        if(nrecv < 0)
        {
            LOG_ERROR("recvfrom error[%s]\n", strerror(errno));
            rt=-1;
            break;
        }
        timeval tv1;
        gettimeofday(&tv1, NULL);
        long offtime=(tv1.tv_sec-tv.tv_sec)*1000000+(tv1.tv_usec-tv.tv_usec);
        LOG_DEBUG("time[%ld]microseconds\n", offtime);
        LOG_DEBUG("nrecv[%d]\n", nrecv);
        if(!rsp.ParseFromArray(buf, nrecv))
        {
            LOG_ERROR("rsp.ParseFromArray error\n");
            rt=-1;
            break;
        }
        LOG_DEBUG("rsp[%s]\n", rsp.DebugString().c_str());

    }while(0);
    return rt;
}

