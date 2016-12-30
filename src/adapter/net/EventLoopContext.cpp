#include "EventLoopContext.h"
#include "net/RedisClient.h"
#include "net/HttpRequest.h"

namespace poseidon
{
namespace adapter
{

//init hash table bucket count
//当前每个eventloop的极限qps差不多5500，加上120ms左右的超时，所以桶数量足够了
EventLoopContext::EventLoopContext() : dsp_request_map(10000)
{
    http_request_destroy = boost::bind(&boost::object_pool<HttpRequest>::destroy,
                                       &http_request_pool, _1);
}

EventLoopContext::~EventLoopContext()
{

}

void EventLoopContext::StoreProtocolObj(const std::string &id, ProtocolObjectPtr &ptr)
{
    dsp_request_map.insert(std::make_pair(id, ptr));
}

void EventLoopContext::OnThreadInit(OnUdpReadCb &cb, muduo::net::EventLoop *loop)
{
    LOG_INFO << "loop thread inited...";
    LoopContextPtr loop_cxt(new EventLoopContext);
    boost::shared_ptr<UdpChannel> instance(new UdpChannel(cb));
    loop->setContext(loop_cxt);
    loop_cxt->udp_ptr = instance;
    //loop_cxt->redis_client.reset(new RedisClient(loop));
    loop_cxt->object_factory.OnWorkThreadInit();
    instance->Start(loop);
}



}
}
