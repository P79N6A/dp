#ifndef CTLADDRGETTER_H
#define CTLADDRGETTER_H

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Atomic.h>

/*
auth:xxxx
date:20160810
desc:负责定时从ha中获取ip地址，尽量避免锁
*/

namespace poseidon
{
namespace adapter
{

class CtlAddrGetter : public boost::noncopyable
{
public:
    explicit CtlAddrGetter(muduo::net::EventLoop*);
    int GetAddr(struct sockaddr_in & addr);
    int GetTestAddr(struct sockaddr_in & addr);
private:
    muduo::net::EventLoop *main_loop_;
    muduo::MutexLock locker_;
    muduo::AtomicInt64 idx_indicate_;
    muduo::AtomicInt64 idx_indicate_test_;
    typedef boost::shared_ptr<std::vector<struct sockaddr_in>> AddrListPtr;
    AddrListPtr addr_list_;
    AddrListPtr test_addr_list_;
    void DoUpdate(const std::string servername, AddrListPtr &list);
};


}
}

#endif // CTLADDRGETTER_H
