#include "AdxDispather.h"

#include <sstream>

#include <boost/bind.hpp>
#include <util/func.h>
#include <gflags/gflags.h>
#include <muduo/net/InetAddress.h>

#include "net/HttpRequest.h"
#include "protocolimp/ProtocolObjFactory.h"
#include "poseidon_tanx.pb.h"
#include "conf/Configer.h"
#include "monitor_api.h" //MON_ADD
#include "ha.h"
/*
协议分派对象。负责把不同接入协议内容传给正确的处理对象。
*/


namespace poseidon
{
namespace adapter
{

namespace
{
DECLARE_THREAD_VAR(poseidon::rtb::BidResponse, rtb_resp);
}

extern Configer * volatile configer;
DECLARE_bool(test_addr);

AdxDispather::AdxDispather(const std::string &log_path) : tc_(checker_.GetLoop()),
    ctl_adr_getter_(checker_.GetLoop())
{
    char timeout_log_path[256];
    snprintf(timeout_log_path, sizeof(timeout_log_path), "%s_timeout", log_path.c_str());
    //ctor
    timeout_logging_.reset(new muduo::AsyncLogging("timeout", 320000000));
    timeout_logging_->start();
    ProtocolObjFactory::InitStaticOnce(checker_.GetLoop());
}

AdxDispather::~AdxDispather()
{
    //dtor
}


int AdxDispather::SendUdp(const UdpChannelPtr &udp,
                          const char *content,
                          size_t len,
                          muduo::net::InetAddress &ret_dst,
                          int source)
{
    struct sockaddr_in addr;
    int rt = ctl_adr_getter_.GetAddr(addr);
    if (rt != 0) {
        LOG_ERROR << "HA_GET_ADDR error";
        return -1;
    }
    muduo::net::InetAddress dst(addr);
    LOG_DEBUG << "HA_GET_ADDR:" << dst.toIpPort();

    //muduo::net::InetAddress dst("10.32.50.214", 27600);
    ret_dst = dst;
    return udp->Send(dst, content, len);
}


void AdxDispather::WriteTimeOutLog(RequestMapCache_Iterator &iter,
                                   const std::string &id,
                                   const muduo::net::InetAddress &dst,
                                   int source)
{
    MON_ADD(ATTR_ADAPTER_CONTROLER_TIMEOUT, 1);
    muduo::LogStream ls;
    std::string strtime;
    util::Func::get_time_str(&strtime);
    ls << strtime << " Addr:" << dst.toIpPort() << " Source:" << source <<
       " Trace_Id:" << id << "\n";
    timeout_logging_->append(ls.buffer().data(), (int)ls.buffer().length());
}

void AdxDispather::CheckTimeOut(RequestMapCache &req_map,
                                RtbReqSharedPtr rtb_req_ptr,
                                muduo::net::InetAddress dst)
{
    tc_.onRtbTimeOut();
    const std::string &trace_id = rtb_req_ptr->trace_id();
    //python模拟tanx发包时，bid不唯一，所以...
    RequestMapCache_Iterator iter = req_map.find(trace_id);
    if(iter != req_map.end()) {
        LOG_WARN << "Controler resp timeout! trace_id:" << trace_id;
        WriteTimeOutLog(iter, trace_id, dst, iter->second->GetSource());
        //超时需要给adx返回一个空应答，以免adx认为超时而降低请求的速率
        iter->second->OnFailed();
        req_map.erase(iter);
    }
}

void AdxDispather::FilterSpecialWords(RtbReqSharedPtr &rtb_ptr)
{
    for (int i = 0; i < rtb_ptr->impressions_size(); ++i) {
        auto *imp = rtb_ptr->mutable_impressions(i);
        if (imp->has_video() && imp->video().has_ext() &&
                imp->video().ext().has_content()) {
            auto *content = imp->mutable_video()->mutable_ext()->mutable_content();
            if (content->has_title()) {
                char *p = (char*)content->mutable_title()->c_str();
                adapter_replace_all(p, '\n', ' ');
                adapter_replace_all(p, '`', ' ');
            }
            for (int j = 0; j < content->keywords_size(); ++j) {
                auto *keyword = content->mutable_keywords(j);
                char *p = (char*)keyword->c_str();
                adapter_replace_all(p, '\n', ' ');
                adapter_replace_all(p, '`', ' ');
            }
        }
    }
    //LOG_INFO <<"after FilterSpecialWords:\n" << rtb_ptr->DebugString();
}



void AdxDispather::OnHttpBodyComplete(const char *buf, size_t len, const HttpRequest *request)
{
    muduo::net::TcpConnectionPtr conn = request->GetWConnection().lock();
    if (!conn) {//unreachable!
        LOG_ERROR << "Connection has closed!";
        return;
    }
    muduo::net::EventLoop *loop = conn->getLoop();
    LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());

    const std::string &url = request->HttpRequestUrl();
    //通过factory对象获取协议处理handler
    ProtocolObjectPtr request_object = loop_ctx->object_factory.GetInstance(url);
    LOG_DEBUG << "Http Request:" << len << " Data:[" << std::string(buf, len) <<"]";
    if (!request_object) {
        LOG_ERROR << "Empty object， Invalid request URL:[" << url << "]";
        MON_ADD(ATTR_ADAPTER_INVALID_URL, 1);
        conn->forceClose();//time_wait
        return;
    }
    if (!request_object->ParseFromBuff(request, buf, len)) {
        LOG_ERROR << "Invalid adx protocol data! source:" << request_object->GetSource();
        MON_ADD(ATTR_ADAPTER_CHANNEL_INVALID_PROTOCOL, 1);
        return ;
    }
    //LOG_INFO << "Adx Request. id:" << request_object.Id();
    tc_.onRequest();//流量控制增加请求计数
    //保存connection的弱引用，以便在response中使用
    request_object->SetWeakPtr(request->GetWConnection());
    MON_ADD(ATTR_ADAPTER_ADX_REQUEST_TOTAL, 1);
    std::vector<RtbReqSharedPtr> request_list;//可能存在多广告位的情况！change to ptr_vector?
    int result = request_object->OnRequest(request, request_list);
    if (result != 0) {
        request_object->OnFailed();
        return;
    }

    char rtb_buff[65536];
    for (size_t i = 0; i < request_list.size(); ++i) {
        RtbReqSharedPtr &rtb_request = request_list[i];
        //Ping, 黑名单，反作弊处理
        if (checker_.CheckRequest(*rtb_request.get())) {
            request_object->OnFailed();
            return;
        }
        //过滤掉某些字段的特殊字符
        FilterSpecialWords(rtb_request);

        int len = rtb_request->ByteSize();
        if (len > UDP_AMX_BUFF_LEN) {
            LOG_ERROR << "Udp Packet are great then 65536!";
            request_object->OnFailed();
            return;
        }
        if (!rtb_request->SerializeToArray(rtb_buff, sizeof(rtb_buff))) {
            LOG_ERROR << "Serialize Rtb Request Failed, trace_id:" << rtb_request->trace_id();
            request_object->OnFailed();
            return;
        }

        muduo::net::InetAddress dst;
        UdpChannelPtr &udp = loop_ctx->udp_ptr;
        int n = SendUdp(udp, rtb_buff, len, dst, request_object->GetSource());
        if (n > 0) {
            //等待应答超时。以后自动清除map中加入的项目
            muduo::net::TimerId timeid = loop->runAfter(
                                             request_object->ResopnseTimeOut(),
                                             boost::bind(&AdxDispather::CheckTimeOut, this,
                                                     boost::ref(loop_ctx->dsp_request_map),
                                                     rtb_request,//rtb_request生命周期是timeout长度
                                                     dst)
                                         );
            request_object->AddTimeIds(rtb_request->trace_id(), timeid);
            loop_ctx->StoreProtocolObj(rtb_request->trace_id(), request_object);
            MON_ADD(ATTR_ADAPTER_CONTROL_REQ_TOTAL, 1);
            //LOG_INFO << "**HashMap**.size:" << loop_ctx->dsp_request_map.size();
        } else {
            LOG_ERROR << "SendUdp Failed, trace_id:" << rtb_request->trace_id();
            request_object->OnFailed();
            break;
        }
    }
}


void AdxDispather::OnHttpRequestComplete(const HttpRequest *request)
{
    //not imp
    MON_ADD(ATTR_ADAPTER_HTTP_REQUEST_TOTAL, 1);
}

void AdxDispather::OnDspResponse(UdpChannel*, muduo::net::EventLoop *loop,
                                 muduo::net::InetAddress &peer, const char *buf, int len)
{
    if (rtb_resp == NULL) {
        rtb_resp = new poseidon::rtb::BidResponse;
    }
    poseidon::rtb::BidResponse &rtb_response = *rtb_resp;
    rtb_response.Clear();
    //解析从dsp返回的应答，保存在rtb_response
    if (rtb_response.ParseFromArray(buf, len)) {
        MON_ADD(ATTR_ADAPTER_CONTROL_RESP_TOTAL, 1);
        LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());
        //LOG_INFO << "RTB Resp. id:" << rtb_response.id();
        LOG_DEBUG << "[-->Rtb Response<--]\n" << rtb_response.DebugString();
        //根据trace_id找到原会话.
        RequestMapCache_Iterator iter = loop_ctx->dsp_request_map.find(rtb_response.trace_id());
        if (iter != loop_ctx->dsp_request_map.end()) {
            //attention:变量引用并不能增加shared_ptr的引用计数
            boost::shared_ptr<ProtocolObject> &request_object = iter->second;

            muduo::net::TimerId timeid = request_object->GetTimeOutId(rtb_response.trace_id());
            loop->cancel(timeid);//取消超时定时器
            if (request_object->RtbResponse(rtb_response, loop) != 0) {
                request_object->OnFailed();
            }
            //应答消息回来后，需要把hashmap保存的信息删除
            loop_ctx->dsp_request_map.erase(iter);
        } else {
            LOG_DEBUG << "Can't find Req object from hashmap. trace_id:" << rtb_response.trace_id() <<
                      ". Maybe has been erased from hashmap by timeout proc!";
        }

    } else {
        LOG_ERROR << "Invalid dsp response data format!";
    }
}

}
}

