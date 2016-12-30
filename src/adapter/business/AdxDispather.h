#ifndef DSPDISPATCHER_H
#define DSPDISPATCHER_H

/**
    auth: xxxx
    date: 2016/05
**/

#include <string>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <muduo/base/AsyncLogging.h>

#include "business/ResquestChecker.h"
#include "business/TrafficControl.h"
#include "net/CtlAddrGetter.h"
#include "net/EventLoopContext.h"


namespace muduo
{
namespace net
{
class EventLoop;
class InetAddress;
}
}

namespace poseidon
{

namespace adapter
{


class HttpRequest;
class AdxDispather : boost::noncopyable
{
public:
    AdxDispather(const std::string &log_path);
    virtual ~AdxDispather();

    void OnHttpBodyComplete(const char*, size_t,
                            const HttpRequest *request);
    void OnHttpRequestComplete(const HttpRequest*);
    void OnDspResponse(UdpChannel*, muduo::net::EventLoop *,
                       muduo::net::InetAddress&, const char*, int len);
    void CheckTimeOut(RequestMapCache&, RtbReqSharedPtr, muduo::net::InetAddress);
private:
    ResquestChecker checker_;
    TrafficControl tc_;
    CtlAddrGetter ctl_adr_getter_;
    boost::scoped_ptr<muduo::AsyncLogging> timeout_logging_;
    void FilterSpecialWords(RtbReqSharedPtr &rtb_ptr);
    int SendUdp(const UdpChannelPtr&, const char *, size_t, muduo::net::InetAddress &ret_dst, int);
    void WriteTimeOutLog(RequestMapCache_Iterator&, const std::string&,
                                   const muduo::net::InetAddress &dst,
                                   int source);
};

}
}

#endif // DSPDISPATCHER_H
