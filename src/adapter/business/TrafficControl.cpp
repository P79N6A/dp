#include "TrafficControl.h"
#include "conf/Configer.h"
#include "protocolimp/ProtocolObject.h"

#include <boost/bind.hpp>

using namespace poseidon;
using namespace poseidon::adapter;


extern Configer * volatile configer;

//main_loop_不再是主线程
TrafficControl::TrafficControl(muduo::net::EventLoop *loop) : main_loop_(loop),
    last_time_(ProtocolObject::Now()),
    calc_data_(configer->GetProperty<int>("traffic.circle_size", 10))

{
    //ctor
    runevery_ = configer->GetProperty<int>("traffic.runevery", 1);
    LOG_INFO << "traffic.runevery:" << runevery_;
    if (runevery_ <= 0) {
        runevery_ = 1;
    }
    every_timer_ = main_loop_->runEvery(runevery_, boost::bind(&TrafficControl::onInternal, this));
    //main_loop_->setContext(this);
}

TrafficControl::~TrafficControl()
{
    //dtor
}

//每个时间间隔计算一次超时率和qps
void TrafficControl::onInternal()
{
    time_t now = ProtocolObject::Now();//time(NULL);
    int64_t nrequest = adx_request_.get();
    last_time_ = now;

    int64_t qps = nrequest / (int64_t)runevery_;
    int64_t timeout = rtb_timeout_.get();
    char buff[1024];
    snprintf(buff, sizeof(buff), "[%lld] QPS:[%lld] TimeOut:[%lld]", now, qps, timeout);
    LOG_INFO << "QPS:" << qps;
    calc_data_.push_back(std::string(buff));//getCalcData函数也必须同一线程调用，故不需要锁
    adx_request_.getAndSet(0);
    rtb_timeout_.getAndSet(0);
}

void TrafficControl::getCalcData(std::vector<std::string> &vec)
{
    //copy(calc_data_.begin(), calc_data_.end(), std::inserter(vec, vec.end()));
}
