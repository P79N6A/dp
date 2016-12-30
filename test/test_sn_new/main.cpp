#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include "protocol/src/poseidon_proto.h"
#include "util/proto_helper.h"
using namespace poseidon;

#define LOG_ERR(fmt, a...) fprintf(stderr, fmt, ##a ); fprintf(stderr, "\n");
#define TV_SUB_MS(_tse_, _tsb_) \
     ((_tse_.tv_sec * 1e3 + _tse_.tv_usec * 1e-3) - (_tsb_.tv_sec * 1e3 + _tsb_.tv_usec * 1e-3))

void pack(char * buf, int & buflen)
{
    sn::SNRequest req;
    util::ParseProtoFromTextFormatFile("multi_vtype.data", &req);
    std::string str;
    req.SerializeToString(&str);
//    printf("req[%s]\n", req.DebugString().c_str());
    buflen = str.length();
    memcpy(buf, str.c_str(), buflen);
}

void get_sn_addr(char *ip, int port, struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(ip);
    memset(addr->sin_zero, 0x00, sizeof(addr->sin_zero));
}

void test_performance(int argc, char * argv[])
{
    char snd_buf[4096], rcv_buf[4096], *dst_ip;
    int n, total, snd_blen, interval, dst_port, 
    stats_interval[100], stats_total[100];
    int sock = -1;

    ssize_t sz, ms;
    struct timeval tv_s, tv_m, tv_b, tv_e;
    struct sockaddr_in dest_addr;

    if (argc < 4) {
        LOG_ERR("usage:%s <ip> <port> <interval>", argv[0]);
        return;
    }

    dst_ip = argv[1];
    dst_port = atoi(argv[2]);
    interval = strtoul(argv[3], NULL, 0);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    get_sn_addr(dst_ip, dst_port, &dest_addr);

    memset(snd_buf, 0x00, sizeof(snd_buf));
    pack(snd_buf, snd_blen);

    gettimeofday(&tv_s, NULL);
    memcpy(&tv_m, &tv_s, sizeof(tv_m));
    memset(stats_interval, 0x00, sizeof(stats_interval));
    memset(stats_total, 0x00, sizeof(stats_total));

    for (n = 1, total = 1;; n++, total++) {
        gettimeofday(&tv_b, NULL);
        sz = sendto(sock, snd_buf, snd_blen, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
        if (sz == -1) {
            fprintf(stderr, "send fail, %s\n", strerror(errno));
        }

        sz = recvfrom(sock, rcv_buf, sizeof(rcv_buf), 0, NULL, NULL);
        if (sz == -1) {
            fprintf(stderr, "send fail, %s\n", strerror(errno));
        }
        gettimeofday(&tv_e, NULL);

        ms = TV_SUB_MS(tv_e, tv_b);
        ms = ms >= 100 ? 99 : ms;
        stats_interval[ms]++;
        stats_total[ms]++;

        ms = TV_SUB_MS(tv_e, tv_m);
        if (ms >= interval * 1000) {
            fprintf(stdout, "\n============STATS==========\n");
            fprintf(stdout, "Stats Interval:\n");
            for(int i = 0; i < 100; i++) {
                if (stats_interval[i] > 0) {
                    fprintf(stdout, "ms[%d]=%d\n", i, stats_interval[i] );
                }
            }
            fprintf(stdout, "QPS:%zd\n\n", n / (ms / 1000));
            n = 0;
            memcpy(&tv_m, &tv_e, sizeof(tv_m));
            memset(stats_interval, 0x00, sizeof(stats_interval));

            fprintf(stdout, "Stats Total:\n");
            for(int i = 0; i < 100; i++) {
                if (stats_total[i] > 0) {
                    fprintf(stdout, "ms[%d]=%d\n", i, stats_total[i] );
                }
            }
            ms = TV_SUB_MS(tv_e, tv_s);
            fprintf(stdout, "QPS:%zd\n", total / (ms / 1000));
        }
    }
}

#if 0
int test_funtion(int argc, char * argv[])
{
    int rt = 0;
    int sock = -1;
    do {
        if (argc < 3) {
            LOG_ERR("usage:%s cnt qps", argv[0]);
            rt = -1;
            break;
        }

        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            break;
        }

//        HA_INIT("100.84.76.79:2181");

        uint64_t cnt = strtoul(argv[1], NULL, 0);
        int qps = strtoul(argv[2], NULL, 0);
        int usleep_time = 1000000/qps;
        char buf[4096];
        int array[100];
        memset(array, 0x0, sizeof(array));
        for (uint64_t i = 0; i < cnt; i++) {
            int buflen = 4096;
            memset(buf, 0x00, 4096);
            pack(buf, buflen);
            struct sockaddr_in addr;
//            HA_GET_ADDR("sn", addr);
            get_sn_addr(&addr);
            uint64_t t1,t2;
 //           poseidon::util::Func::get_time_ms(t1);
            sendto(sock, buf, buflen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
            recvfrom(sock, buf, 4096, 0, NULL, NULL);
  //          poseidon::util::Func::get_time_ms(t2);
   //         int ms = (t2 - t1 >= 100) ? 99 : (t2 - t1);
    //        array[ms]++;
//            usleep(usleep_time);
        }

     //   for(int i=0; i<100; i++) {
     //       if (array[i] > 0) {
     //           printf("ms[%d]=%d\n", i, array[i] );
     //       }
     //   }

    } while(0);

    if (sock > 0) {
        close(sock);
    }

    return rt;
}
#endif

int main(int argc, char * argv[])
{
    test_performance(argc, argv);
    return 0;
}
