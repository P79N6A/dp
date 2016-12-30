#include <iostream>
#include <string>
#include <sys/file.h>
#include <sys/stat.h>

#include <muduo/base/Logging.h>
#include <muduo/base/AsyncLogging.h>
#include <muduo/base/LogFile.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <boost/scoped_ptr.hpp>
#include <gflags/gflags.h>

#include "conf/Configer.h"
#include "net/TcpHandler.h"
#include "net/EventLoopContext.h"
#include "business/AdxDispather.h"
#include "monitor.h"
#include "ha.h"
#include "util/func.h"


/**
    auth: xxxx
    date: 2016/05
**/

using namespace std;
using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;
using namespace boost;

Configer * volatile configer;

#ifdef ASYNLOG
#define LOGFILE AsyncLogging
#else
#define LOGFILE LogFile
#endif

boost::scoped_ptr<LOGFILE> g_logfile;
static void LogOutput(const char* msg, int len);
static void FlushLog();
static int CheckDualInstance(int);

DEFINE_string(conf, "../conf/adapter.conf", "programe cfg path.");
DEFINE_string(log, "", "programe log output dir.");
DEFINE_string(pid, "/tmp/adapter.lock", "pid file dir.");
DEFINE_bool(zk, true, "for debug!");
DEFINE_bool(test_addr, true, "for debug!");

int main(int argc, char **argv)
{
    google::SetVersionString("1.8.2.2");
    google::SetUsageMessage("Usage : some help message ");
    google::ParseCommandLineFlags(&argc, &argv, true);
    //pthread_attr_setstack
    poseidon::util::Func::DaemonInit();
    while(1) {
        EventLoop accepter;//accepter放到最后析构，因为configer需要在析构时使用eventloop
        Configer adapter_cfg(FLAGS_conf);//配置类
        configer = &adapter_cfg;
        //检查是否重复启动！
        if(CheckDualInstance(adapter_cfg.port()) < 0) {
            return -1;
        }

        if(FLAGS_zk) {
            std::string zk_addr = configer->GetProperty<std::string>("net.ZK_address", "");
            int rt=HA_INIT(zk_addr);//ZK addr
            if(rt != 0) {
                LOG_ERROR << "ha_init error";//default stdout
                break;
            }
        }
        ////////////////////////////设置log级别////////////////
        Logger::setLogLevel(Configer::translatelog(adapter_cfg.loglevel().c_str()));
        std::string log_path = FLAGS_log;//configer->GetProperty<std::string>("base.log_output", "");
        if(log_path.length() > 0 && log_path.at(log_path.length() -1) != '/') {
            log_path += "/";
        }
        log_path += "poseidon_adapter";
#ifdef ASYNLOG
        g_logfile.reset(new AsyncLogging(log_path.c_str(),
                                         configer->GetProperty<int>("base.logsize", 655350000)));
        g_logfile->start();
#else
        g_logfile.reset(new LogFile(log_path, configer->GetProperty<int>("base.logsize", 655350000), true, -1));
#endif
        Logger::setOutput(LogOutput);
        Logger::setFlush(FlushLog);
        ////////////////////////////end of loger///////////////////
        //配置udp命令接收器
        LOG_INFO << "------Starting UDP Command Server...------";
        InetAddress udpaddr(adapter_cfg.port());
        OnUdpReadCb cfgread = boost::bind(&Configer::UdpRead, configer,
                                          _1, _2, _3, _4, _5);
        scoped_ptr<UdpServerChannel> config_chanl(new UdpServerChannel(cfgread));
        config_chanl->DoBind(udpaddr);
        config_chanl->Start(&accepter);//主线程作为udpserver的io线程
        LOG_INFO << "------UDP Command Server start success!------";
        ////////////////////////////end of udp////////////////
        AdxDispather adx(log_path);//初始化协议对象分派接口
        OnBodyComplete bdycomp = boost::bind(&AdxDispather::OnHttpBodyComplete,
                                             &adx, _1, _2, _3);
        OnRequestComplete reqcomp = boost::bind(&AdxDispather::OnHttpRequestComplete,
                                                &adx, _1);
        //AdxDispather::OnHttpBodyComplete，AdxDispather::OnHttpRequestComplete
        //这两个函数委托给HttpRequest的两个http响应事件做处理器
        TcpHandler tcphandler(bdycomp, reqcomp);

        LOG_INFO << "Port:" << adapter_cfg.port();
        LOG_INFO << "Thread Num:" << adapter_cfg.threads();
        LOG_INFO << "LogLevel:" << adapter_cfg.loglevel();

        LOG_INFO << "------Starting Adapter Server...------";
        InetAddress addr(adapter_cfg.port());
        TcpServer server(&accepter,addr, "adapterserver");
        server.setThreadNum(adapter_cfg.threads());
        server.setConnectionCallback(bind(&TcpHandler::OnConnection, &tcphandler, _1));
        server.setMessageCallback(bind(&TcpHandler::OnMessage, &tcphandler, _1, _2, _3));
        //把AdxRequest::OnDspResponse作为udp读数据的回调
        OnUdpReadCb rcb = boost::bind(&AdxDispather::OnDspResponse, &adx, _1, _2, _3, _4, _5);
        server.setThreadInitCallback(bind(&EventLoopContext::OnThreadInit, boost::ref(rcb), _1));
        server.start();
        LOG_INFO << "------Adapter Server start success!------";

        accepter.loop();
    }
    return 0;
}

static int CheckDualInstance(int port)
{
    //char filename[256];
    //if(FLAGS_pid.at(FLAGS_pid.length() -1) != '/')
    //    FLAGS_pid += "/";
    //snprintf(filename, sizeof(filename), "%sposeidon_adapter.%d.lock", FLAGS_pid.c_str(), port);
    int lock_file = ::open(FLAGS_pid.c_str(), O_CREAT|O_RDWR, 0666);
    if(lock_file <= 0) {
        perror("invalid pid path:");
        return -2;
    }
    int rc = ::flock(lock_file, LOCK_EX|LOCK_NB);
    if (rc) {
        if (EWOULDBLOCK == errno) {
            printf("Dual Instance! Exit...\n");
            ::close(lock_file);
            return -1;
        }
    } else {
        char buffer[64];
        sprintf(buffer, "%d", getpid());
        write(lock_file, buffer, strlen(buffer));
        printf("Startup new instance!\n");
    }
    return lock_file;
}

static void LogOutput(const char* msg, int len)
{
    g_logfile->append(msg, len);
}

static void FlushLog()
{
#ifndef ASYNLOG
    g_logfile->flush();
#endif
}
