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
    //���е�Http�����װ���������Ϻ�ί�и�ȫ�ֵ�AdxRequest������Ӧ�ķ�������
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
        //�ó��̰߳󶨵�LoopContextPtr
        muduo::net::EventLoop *loop = conp->getLoop();
        LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());

        //ÿ�����Ӷ�Ӧһ��httprequestʵ��
        boost::shared_ptr<HttpRequest> ptr(loop_ctx->http_request_pool.construct(conp),
                                           loop_ctx->http_request_destroy);
        //�����б���HttpRequest�������httprequest������������TcpConnectio��ͬ
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
    //�Ƿ����������ݣ�
    if(proceed == 0) {
        MON_ADD(ATTR_ADAPTER_HTTP_INVALLID_HEAD, 1);
        conp->forceClose(); //! time_wait state !
        return;
    }
    buf->retrieve(proceed);
    //���û���
    if(buf->readableBytes() == 0) {
        buf->retrieveAll();
    }
}


