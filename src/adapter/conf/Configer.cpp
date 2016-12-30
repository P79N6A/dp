#include "Configer.h"
#include "net/UdpChannel.h"
#include "utility/StringSplitTools.h"
#include "business/TrafficControl.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <muduo/base/Logging.h>

/**
    auth: xxxx
    date: 2016/05
**/

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo::net;

Configer::Configer(const std::string &cfgfile) : cfgfilename_(cfgfile)

{
    Init();
}

Configer::~Configer()
{
    //dtor
}

//loop目前为主线程
void Configer::UdpRead(UdpChannel *udp, EventLoop *loop,
                       InetAddress &addr, const char *buff, int len)
{
    LOG_INFO << "Recv UDP Command from :" << addr.toIp();
    if (len > 255 || len <= 0) {
        LOG_ERROR << "UDP length is greater 255!";
        return;
    }
    char request[256];
    memcpy(request, buff, len);
    if (request[len - 1] == '\n') {//去除回车键
        request[len - 1] = '\0';
        len -= 1;
    }

    char buf[256] = { 0 };
    if(memcmp(request, "update", 6) == 0) {
        Init();
        int n = snprintf(buf, sizeof(buf), "%s", "update config success!\n");
        udp->Send(addr, buf, n);
    } else if(memcmp(request, "restart", 7) == 0) {
        return;//do not restart!
        int n = snprintf(buf, sizeof(buf), "%s", "restart success!\n");
        udp->Send(addr, buf, n);
        loop->quit();//重新启动程序！
    } else if(memcmp(request, "info", 4) == 0) {
        TrafficControl *tc = boost::any_cast<TrafficControl*>(loop->getContext());
        std::vector<std::string> vec;
        tc->getCalcData(vec);
        std::string result;
        for(size_t i = 0; i < vec.size(); ++i)
        {
            result += vec[i];
            result += "\n";
            if(result.length() > 65000) break;
        }
        LOG_INFO << "Recv QPS_info cmd:\n" << result;//此时调用LOG是安全的
        udp->Send(addr, result);

    } else if(memcmp(request, "log", 3) == 0) {
        const char *p = request + 3;
        while(*p == ' ') {
            ++p;
        }
        const char *s = p;
        while(*p != '\0' && *p != ' ' && p <= request + len) {
            ++p;
        }
        char loglel[32] = { 0 };
        memcpy(loglel, s, p - s);
        if(strlen(loglel) == 0) {
            return;
        }
        muduo::Logger::LogLevel old_level = muduo::Logger::logLevel();
        muduo::Logger::LogLevel level = Configer::translatelog(loglel);
        LOG_INFO << "Change LogLevel to:" << loglel << ". translog result:" << level;
        muduo::Logger::setLogLevel(level);
        muduo::Logger::LogLevel new_level = muduo::Logger::logLevel();
        if (old_level != new_level) {
            char retbuff[32];
            strcpy(retbuff, "Change LogLevel Success!\n");
            udp->Send(addr, retbuff);
        }
    } else {
        std::string result = "Unknow Command!\n";
        udp->Send(addr, result);
    }
}

void Configer::Init()
{
    cfgtree_.clear();//Clear this tree completely, of both data and children
    boost::property_tree::ini_parser::read_ini(cfgfilename_, cfgtree_);
    std::string bfkey = cfgtree_.get<std::string>("base.encrypt_key","");
    //初始化bf_key
    //BF_set_key(&bf_key, bfkey.length(), (const unsigned char*)bfkey.c_str());

    std::vector<std::string> vectemp;
    filter_imei_.clear();
    std::string filter_imei = cfgtree_.get<std::string>("base.filter_imei","");
    if(filter_imei.length() > 0) {
        StringSplitTools::splitString(filter_imei, ':', vectemp);
        copy(vectemp.begin(), vectemp.end(), inserter(filter_imei_, filter_imei_.end()));
    }
}

int Configer::maxfd()
{
    return cfgtree_.get<int>("net.max_fd_limit", 1021);
}

short int Configer::port()
{
    return cfgtree_.get<short int>("net.listen_port", 8899);
}

int Configer::threads()
{
    return cfgtree_.get<int>("io.loop_thread", 4);
}

std::string Configer::loglevel()
{
    return cfgtree_.get<std::string>("base.loglevel","INFO");
}

muduo::Logger::LogLevel Configer::translatelog(const char *logstr)
{
    if(strcmp(logstr, "TRACE") == 0) {
        return muduo::Logger::TRACE;
    } else if(strcmp(logstr, "DEBUG") == 0) {
        return muduo::Logger::DEBUG;
    } else if(strcmp(logstr, "INFO") == 0) {
        return muduo::Logger::INFO;
    } else if(strcmp(logstr, "WARN") == 0) {
        return muduo::Logger::WARN;
    } else if(strcmp(logstr, "ERROR") == 0) {
        return muduo::Logger::ERROR;
    } else if(strcmp(logstr, "FATAL") == 0) {
        return muduo::Logger::FATAL;
    }
    return muduo::Logger::INFO;
}

double Configer::requesttimeout()
{
    int ms = cfgtree_.get<int>("net.request_timeout", 2000);
    return ((double)ms / 1000);
}

/*
double Configer::responsetimeout()
{
    int ms = cfgtree_.get<int>("net.response_timeout", 100);
    return ((double)ms / 1000);
}
*/

std::string  Configer::sql_host()
{
    return cfgtree_.get<std::string>("sql.host", "");
}

std::string  Configer::sql_user()
{
    return cfgtree_.get<std::string>("sql.user", "");
}

std::string  Configer::sql_pass()
{
    return cfgtree_.get<std::string>("sql.pass", "");
}

int Configer::sql_port()
{
    return cfgtree_.get<int>("sql.port", 0);
}

std::string  Configer::sql_db()
{
    return cfgtree_.get<std::string>("sql.db_name", "");
}

int  Configer::refresh_internal()
{
    return cfgtree_.get<int>("sql.refresh_internal", 2);
}

std::string Configer::dspid()
{
    return cfgtree_.get<std::string>("base.dspid", "");
}

int Configer::Get_Bf_Key_Ver()
{
    return cfgtree_.get<int>("base.encrypt_key_version", 0);
}


std::string Configer::GetString(const std::string &path)
{
    return cfgtree_.get<std::string>(path.c_str());
}

int Configer::GetInt(const std::string &path)
{
    return cfgtree_.get<int>(path.c_str());
}

Configer::Pair_List Configer::GetSectionKeys(const std::string &_path)
{
    Configer::Pair_List child;
    try {
        auto &x = cfgtree_.get_child(_path);
        auto pos = x.begin();
        for(; pos != x.end(); ++pos) {
            child.push_back(std::make_pair(pos->first, pos->second.data()));//value_type
            //LOG_INFO << "key:" << pos->first << " value:" << pos->second.data();
        }
    } catch(std::exception&) {
        //donothing
    }
    return std::move(child);
}





