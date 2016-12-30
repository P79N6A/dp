#ifndef UDPSERVER_H
#define UDPSERVER_H
/**
    auth:xxxx
    date:2016/05
    desc:负责发送/接收controler端请求
**/
#include <time.h>

#include <string>
#include <map>
#include <queue>
#include <muduo/base/Timestamp.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Channel.h>
#include <muduo/net/InetAddress.h>
#include "common/common.h"

namespace poseidon {

namespace adapter {

class UdpChannel;
typedef boost::function<void(UdpChannel*, muduo::net::EventLoop*,
                             muduo::net::InetAddress&, const char*, int)> OnUdpReadCb;
class UdpChannel : boost::noncopyable
{
public:
    UdpChannel(OnUdpReadCb &cb);
    virtual ~UdpChannel();
    void Start(muduo::net::EventLoop *loop);

    void OnUdpRead(muduo::net::EventLoop*, muduo::Timestamp tm);
    void OnUdpWrite();
    int Send(const muduo::net::InetAddress &, const std::string&);
    int Send(const muduo::net::InetAddress &, const char*, int);
private:
    OnUdpReadCb buffcb_;
    boost::shared_ptr<muduo::net::Channel> channel_;
    typedef struct {
        muduo::net::Buffer buff;
        muduo::net::InetAddress dst;
    } UdpSegment;
    std::queue<UdpSegment> udp_send_queue_;
protected:
    int fd_;
};

class UdpServerChannel : public UdpChannel
{
public:
    UdpServerChannel(OnUdpReadCb &cb) : UdpChannel(cb)
    {

    }
    void DoBind(const muduo::net::InetAddress &addr);
};


}
}

#endif // UDPSERVER_H
