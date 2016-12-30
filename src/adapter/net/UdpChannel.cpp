#include "UdpChannel.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include <muduo/base/Logging.h>

/*
auth: zhangxh
date: 2016-06
*/

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo::net;
namespace poseidon
{

namespace adapter
{


UdpChannel::UdpChannel(OnUdpReadCb &cb) : buffcb_(cb)
{
    //ctor
    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd_ <= 0) {
        LOG_ERROR << "Alloc udp fd failed!!";
    }
    //set recvbuf
    //modify: /etc/sysctl.conf   add:  net.core.rmem_max = 8192000 rmem_default = ...
    int buffsize = 6553600;
    setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &buffsize, sizeof(int));
    //set nonblock
    int flags = fcntl(fd_, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd_, F_SETFL, flags);
}

UdpChannel::~UdpChannel()
{
    if(channel_) {//channel析构前，必须先调用以下两个函数！
        channel_->disableAll();
        channel_->remove();//如不调用此函数，则channel_析构时会断言"!addedToLoop_"错误
        //在io event外释放channel
        void (boost::shared_ptr<muduo::net::Channel>::*ptr_reset)();
        ptr_reset = &boost::shared_ptr<muduo::net::Channel>::reset;
        //延长channel_生命周期
        boost::function<void()> fn = boost::bind(ptr_reset, channel_);
        channel_->ownerLoop()->queueInLoop(fn);
    }
    //dtor
    if(fd_) {
        close(fd_);
    }
}


void UdpChannel::Start(muduo::net::EventLoop *loop)
{
    channel_.reset(new muduo::net::Channel(loop, fd_));
    channel_->setReadCallback(boost::bind(&UdpChannel::OnUdpRead, this, loop, _1));
    channel_->setWriteCallback(boost::bind(&UdpChannel::OnUdpWrite, this));
    channel_->enableReading();
}

int UdpChannel::Send(const muduo::net::InetAddress &addr, const std::string &pb)
{
    return Send(addr, pb.c_str(), static_cast<int>(pb.length()));
}

int UdpChannel::Send(const muduo::net::InetAddress &addr, const char *buf, int len)
{
    const struct sockaddr *dst = addr.getSockAddr();
    int ret = sendto(fd_, buf, len, 0, dst, static_cast<socklen_t>(sizeof(struct sockaddr)));
    if(ret <= 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            if(udp_send_queue_.size() > 65535) {
                LOG_ERROR << "Udp send queue is too long!!";
                return ret;
            }
            //LOG_INFO << "Padding UDP Segmeng... paded size:" << udp_send_queue_.size();
            udp_send_queue_.push(UdpSegment());
            UdpSegment &back = udp_send_queue_.back();//取出刚push的引用
            back.buff.append(buf, len);
            back.dst = addr;
            channel_->enableWriting();//打开读
            ret = len;
        } else {
            LOG_ERROR << "udp_sendto " << addr.toIp() << " failed:" << strerror(errno);
        }
    }
    return ret;
}

void UdpChannel::OnUdpWrite()
{
    //LOG_INFO << "OnUdpWrite begin.paded size:" << udp_send_queue_.size();
    while(udp_send_queue_.size() > 0) {
        //从队列第一个开始发送
        UdpSegment &udp_seg = udp_send_queue_.front();
        const struct sockaddr *dst = udp_seg.dst.getSockAddr();
        Buffer &buffer = udp_seg.buff;
        int ret = sendto(fd_, buffer.peek(), buffer.readableBytes()
                         , 0, dst, static_cast<socklen_t>(sizeof(struct sockaddr)));
        if(ret > 0) {
            udp_send_queue_.pop();//成功发送就继续下一个
        } else {
            break;
        }
    }
    //LOG_INFO << "OnUdpWrite end.paded size:" << udp_send_queue_.size();
    if(udp_send_queue_.size() == 0) {
        channel_->disableWriting();//不需要发送堆积的数据了，关闭读事件
    }
}

void UdpChannel::OnUdpRead(muduo::net::EventLoop *loop, muduo::Timestamp tm)
{
    char buff[65536];
    struct sockaddr_in peer = { 0 };
    socklen_t len = sizeof(peer);
    int n = recvfrom(fd_, buff, sizeof(buff), 0, (struct sockaddr*)&peer, &len);
    if(n <= 0) {
        return;
    }
    //buff[n] = '\0';//not nessary
    muduo::net::InetAddress addr(peer);
    //LOG_DEBUG << "Dsp Resp: " << addr.toIpPort() << ":-size:" << n;
    LOG_TRACE << buff;
    //call back
    if(buffcb_) {
        try {
            buffcb_(this, loop, addr, buff, n);
        } catch(const std::exception &e) {
            LOG_ERROR << "OnUdpRead raise error!" << e.what();
        } catch( ... ) {
            LOG_ERROR << "OnUdpRead raise unknow error!";
        }
    }
}


void UdpServerChannel::DoBind(const muduo::net::InetAddress &addr)
{
    int ret = bind(fd_, addr.getSockAddr(), sizeof(struct sockaddr));
    if(ret != 0) {
        LOG_ERROR << "bind error!";
    }
}

}
}
