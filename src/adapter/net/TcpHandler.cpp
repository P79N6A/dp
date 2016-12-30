#include "TcpHandler.h"
#include "conf/Configer.h"
#include "net/EventLoopContext.h"
#include "monitor_api.h"//MON_ADD

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <muduo/base/Logging.h>
/**
    auth: xxxx
    date: 2016/05
**/

using namespace poseidon;
using namespace poseidon::adapter;

extern Configer * volatile configer;

TcpHandler::TcpHandler(OnBodyComplete &bodycompleteCb, OnRequestComplete &requestcompleteCb)
{
    double request_time_out = configer->requesttimeout();
    HttpRequest::SetHttpRequestTimeOut(request_time_out);
    //所有的Http请求包装对象解析完毕后委托给全局的AdxRequest对象相应的方法处理
    HttpRequest::SetBodyCompleteCb(bodycompleteCb);
    HttpRequest::SetRequestCompleteCb(requestcompleteCb);
}

TcpHandler::~TcpHandler()
{
    //dtor
}

void TcpHandler::OnConnection(const muduo::net::TcpConnectionPtr &conp)
{
    if(conp->connected()) {
        if(connections_.get() < configer->maxfd()) {
            connections_.increment();
        } else {
            conp->forceClose();//! time_wait state !
            return;
        }
        //LOG_INFO << "new connection:" << conp->peerAddress().toIpPort();
        //拿出线程绑定的LoopContextPtr
        muduo::net::EventLoop *loop = conp->getLoop();
        LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());

        //每个连接对应一个httprequest实例
        boost::shared_ptr<HttpRequest> ptr(loop_ctx->http_request_pool.construct(conp),
                                           loop_ctx->http_request_destroy);
        //连接中保存HttpRequest对象，因此httprequest的生命周期与TcpConnectio相同
        conp->setContext(ptr);
    } else {
        //LOG_INFO << "connection closed:" << conp->peerAddress().toIpPort();
        connections_.decrement();
    }
}

void TcpHandler::OnMessage(const muduo::net::TcpConnectionPtr &conp,
                           muduo::net::Buffer *buf,
                           muduo::Timestamp time)
{
    boost::shared_ptr<HttpRequest> request =
        boost::any_cast<boost::shared_ptr<HttpRequest>>(conp->getContext());
    size_t proceed = 0;
    try {
        proceed = request->ParseHttp(conp->peerAddress(), buf->peek(), buf->readableBytes());
    } catch(const std::exception &e) {
        LOG_ERROR << "ParseHttp raise error!" << e.what();
    } catch( ... ) {
        LOG_ERROR << "ParseHttp raise unknow error!";
    }
    //非法的请求数据！
    if(proceed == 0) {
        MON_ADD(ATTR_ADAPTER_HTTP_INVALLID_HEAD, 1);
        conp->forceClose(); //! time_wait state !
        return;
    }
    buf->retrieve(proceed);
    //重置缓冲
    if(buf->readableBytes() == 0) {
        buf->retrieveAll();
    }
}


