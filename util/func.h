/**
 **/

#ifndef  _UTIL_FUNC_H_
#define  _UTIL_FUNC_H_
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "third_party/cityhash/include/city.h"

namespace poseidon {
namespace util {

#define MakeAddr(toaddr, ip, port)  do{ \
    toaddr.sin_family=AF_INET;   \
    toaddr.sin_port=htons(port);    \
    inet_aton(ip, &toaddr.sin_addr);    \
}while(0)

class Func {
public:
    static void get_time_info(struct tm & tminfo) {
        time_t now = time(NULL);
        localtime_r(&now, &tminfo);
    }

    static void get_time_ms(uint64_t & timems) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        timems = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }

    static void get_time_str(std::string* time_str, const char * fmt =
            "%Y-%m-%d %H:%M:%S") {
        char str[32];
        struct tm timeinfo;
        time_t now = time(NULL);
        localtime_r(&now, &timeinfo);
        strftime(str, 32, fmt, &timeinfo);
        time_str->assign(str);
    }

    /**
     * @brief               姣杞瑃imeval, 鐢ㄤ簬涓�浜涜秴鏃惰缃�
     * @param time_ms       [IN], 姣鏁�
     * @param tv            [OUT], 杩斿洖timeval
     **/
    void ms_to_timeval(int timems, struct timeval & tv) {
        tv.tv_sec = timems / 1000;
        tv.tv_usec = (timems - tv.tv_sec * 1000) * 1000;
    }

    static const std::string to_str(int val) {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }

    static const std::string to_str(uint32_t val) {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }

    static const std::string to_str(uint64_t val) {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }

    static const std::string to_str(float val, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << val;
        return ss.str();
    }

    static const std::string to_str(double val, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << val;
        return ss.str();
    }

    static const std::string to_str(const struct sockaddr_in & addr) {
        std::stringstream ss;
        ss << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
        return ss.str();
    }

    static int to_int(std::string val) {
        return atoi(val.c_str());
    }

    /**
     * @brief               鑾峰彇鏈満IP
     * @param ip            [OUT], 杩斿洖IP
     * @return              success return 0, or return other
     **/
    static int get_local_ip(std::string & ip);

    static bool single_instance(const std::string & pid_file);

    /**
     * @brief               鏍规嵁鏂囦欢璁＄畻md5鍊�
     **/
    static int md5sum(const std::string & filename, std::string & md5str);

    static void DaemonInit();
    static bool BinaryFileMD5(const char *file_path, std::string *md5);
    static uint32_t BytesHash32(const char *buf, size_t len) {
        return CityHash64(buf, len) >> 32;
    }

    static uint64_t BytesHash64(const char *buf, size_t len) {
        return CityHash64(buf, len);
    }

    static void write_pid_file(std::string path);
    //crc16,copy from hiredis-vip
    static uint16_t crc16(const char *buf, int len);
    //crc32,copy from php
    static uint32_t crc32(const char *buf, int len);
    static bool md5sum(const char * buff, int buff_len, std::string &md5_str);

    static bool gen_addr(int port, struct sockaddr_in & addr,
            socklen_t &addr_len);
    static bool gen_addr(const std::string &host, int port,
            struct sockaddr_in & addr, socklen_t &addr_len);
};

}
}

#endif   // ----- #ifndef _UTIL_FUNC_H_  -----

