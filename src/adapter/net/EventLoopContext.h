#ifndef EVENTLOOPCONTEXT_H
#define EVENTLOOPCONTEXT_H

#include <boost/function.hpp>

#include "utility/DevIdDecoder.h"
#include "net/UdpChannel.h"
#include "protocolimp/ProtocolObjFactory.h"

namespace poseidon
{
namespace adapter
{

class RedisClient;
class HttpRequest;
class UdpChannel;
typedef boost::shared_ptr<UdpChannel> UdpChannelPtr;
//此对象作为muduo::net::eventloop的Context引用保存
class EventLoopContext : boost::noncopyable
{
public:
    EventLoopContext();
    ~EventLoopContext();
    //boost::scoped_ptr<RedisClient> redis_client;
    DevIdDecoder devid_decoder;//每个线程单独的设备id解密对象
    UdpChannelPtr udp_ptr;//Udp传输通道。每个eventloop对应一个，用来与controler通信
    ProtocolObjFactory object_factory;//业务对象工厂对象
    boost::function<void(HttpRequest*)> http_request_destroy;
    boost::object_pool<HttpRequest> http_request_pool;
    //boost::object_pool对象必须在dsp_request_map后释放。因为hash_map析构时会clear容器，
    //此时容器会调用object_pool的destroy方法来释放池对象，因此必须保证object_pool对象存在！
    RequestMapCache dsp_request_map;//每个EventLoop都持有一个hashmap，保存每个请求的cookie信息
    void StoreProtocolObj(const std::string &id, ProtocolObjectPtr &ptr);
    //end 对象池
    static void OnThreadInit(OnUdpReadCb&, muduo::net::EventLoop *loop);
};


typedef boost::shared_ptr<EventLoopContext> LoopContextPtr;

}
}


#endif
