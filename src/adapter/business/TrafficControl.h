#ifndef TRAFFICCONTROL_H
#define TRAFFICCONTROL_H

#include <algorithm>
#include <vector>

#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>
#include <muduo/base/Atomic.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
//#include <muduo/base/Mutex.h>
/*
auth: xxxx
date: 2016/07/04
*/

namespace poseidon
{
namespace adapter
{

class TrafficControl : boost::noncopyable
{
public:
    explicit TrafficControl(muduo::net::EventLoop *loop);
    ~TrafficControl();

    void onInternal();
    void onRtbTimeOut() { rtb_timeout_.increment(); }
    void onRequest() { adx_request_.increment(); }
    void getCalcData(std::vector<std::string>&);
private:
    muduo::net::EventLoop *main_loop_;
    muduo::net::TimerId every_timer_;
    time_t last_time_;
    int runevery_;
    //muduo::MutexLock locker_;
    int cicle_size_;
    boost::circular_buffer<std::string> calc_data_;
    muduo::AtomicInt64 adx_request_;//请求计数
    muduo::AtomicInt64 rtb_timeout_;//rtb超时计数
};

}
}

#endif // TRAFFICCONTROL_H
