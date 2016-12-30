#include "MaMaPBObject.h"
#include "TanxAdapter.h"
#include "TanxAdapter.h"
#include "net/EventLoopContext.h"

#include <boost/algorithm/string.hpp>




using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;

extern Configer * volatile configer;

std::string MaMaPBObject::dspid_;
rtb::TrafficSource MaMaPBObject::source_ = rtb::TS_TANX;
std::string MaMaPBObject::click_url_;
std::string MaMaPBObject::expose_url_;
std::string MaMaPBObject::downloaded_url_;
double MaMaPBObject::mama_resp_timeout_;
std::map<std::string, int> MaMaPBObject::pid_viewtype_map_;

namespace
{
DECLARE_THREAD_VAR_LIST(tanx::BidRequest, mama_req);
}

MaMaPBObject::MaMaPBObject() : mama_tanx_request_(*Alloc_Bid_Obj(THREAD_VAR_LIST(mama_req)))
{
    //ctor
}

MaMaPBObject::~MaMaPBObject()
{
    //dtor
    DestroyObject(&mama_tanx_request_, THREAD_VAR_LIST(mama_req));
}

void MaMaPBObject::OnThreadInitStatic()
{
    InitThreadedQueuePool(THREAD_VAR_LIST(mama_req));
}

void MaMaPBObject::InitStaticVar()
{
    dspid_ = configer->dspid();
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.mama_feedback_url", "");
    click_url_ = host + feedback + "/click?";
    expose_url_ = host + feedback + "/feedback?";
    downloaded_url_ = host + feedback + "/download?";

    int timeout = configer->GetProperty<int>("net.mama_response_timeout", 120);
    mama_resp_timeout_ = (double)timeout / 1000;

    LOG_INFO << "mama_resp_timeout:" << mama_resp_timeout_;
    InitMapViewType();
}


void MaMaPBObject::InitMapViewType()
{
    //key-value
    std::vector<std::pair<std::string, std::string>> viewtype_map =
                configer->GetSectionKeys("tanx_view_type_map");

    for(int i = 0; i < viewtype_map.size(); ++i) {
        std::pair<std::string, std::string> &pairs = viewtype_map[i];
        std::vector<std::string> list;
        boost::split(list, pairs.second, boost::is_any_of(";"));
        LOG_INFO << "viewtype:" << pairs.first << " pidsize:" << list.size();
        auto iter = list.begin();
        for(; iter != list.end(); ++iter) {
            pid_viewtype_map_.insert(std::make_pair(*iter, atoi(pairs.first.c_str())));//pid - viewtype
        }
    }
}



void MaMaPBObject::OnFailed()
{
    BuildEmptyResp(mama_tanx_request_.version(), mama_tanx_request_.bid(), wptr_);
}

bool MaMaPBObject::ParseFromBuff(const HttpRequest*, const char *buff, size_t len)
{
    return mama_tanx_request_.ParseFromArray(buff, len);
}

int MaMaPBObject::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    MON_ADD(ATTR_ADAPTER_TANX_REQUEST, 1);
    if(mama_tanx_request_.is_ping()) {
        //BuildEmptyResp(mama_tanx_request_.version(), mama_tanx_request_.bid(), request->GetWConnection());
        return -1;
    }
    EventLoop *loop = wptr_.lock()->getLoop();
    LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());
    TanxAdapter tanx_adapter(this);
    //把Tanx request转换成内部dsp协议
    if(tanx_adapter.Process(mama_tanx_request_, rtb_request_, loop_ctx) == 0) {
        //mini_req->headers = request->Headers();//保存http请求头！ PVlog使用
        MON_ADD(ATTR_ADAPTER_CONTROLER_TANX_REQUEST, 1);
        rtb_request_->set_trace_id(CreateTraceId());
        rtb_request_list.push_back(rtb_request_);
        return 0;
    } else { //处理失败也返回一个空应答！此应答由AdxRequest中回调OnFailed进行
        //BuildEmptyResp(mama_tanx_request_.version(), mama_tanx_request_.bid(), wptr_);
        return -3;
    }
}

int MaMaPBObject::RtbResponse(poseidon::rtb::BidResponse &rtb_response, EventLoop *loop)
{
    MON_ADD(ATTR_ADAPTER_CONTROLER_TANX_RESPONSE, 1);

    if(rtb_response.no_bid_reason() != 0) {
        //LOG_WARN << "no ads of bid from controller, reason code is:" << rtb_response.no_bid_reason();
        return -1;
    }

    std::string adx_response_str;
    //LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());
    //协议转换
    TanxAdapter tanx_adapter(this);
    if(tanx_adapter.ProcessToTanxResp(rtb_response, adx_response_str) == 0) {
        MON_ADD(ATTR_ADAPTER_TANX_SUCC_RESPONSE, 1);
        AdxResponse(wptr_, adx_response_str, rtb_response.id());
    }//else send a empty response
    else {
        //MON("INVALID_RTB_RESPONSE", 1);
        //LOG_WARN << "Process Rtb Response failed! bid:" << rtb_response.id();
        //BuildEmptyResp(mama_tanx_request_.version(),
        //               rtb_response.id(),//bid
        //              wptr_);
        return -1;
    }
    return 0;
}

//?adx???????
void MaMaPBObject::BuildEmptyResp(int version, const std::string &bid, TcpConnectionWptr wptr)
{
    MON_ADD(ATTR_ADAPTER_TANX_EMPTY_RESPONSE, 1);
    std::string emptyrespon;
    TanxAdapter::buildEmptyTanxResp(version, bid, emptyrespon);
    AdxResponse(wptr, emptyrespon, bid);
}

//?Adx????
void MaMaPBObject::AdxResponse(TcpConnectionWptr &wptr, const std::string &response, const std::string &bid)
{
    static const string simple_resp = "HTTP/1.1 200 OK\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      //"Content-Type:application/octet-stream\r\n"
                                      "Content-Type:application/x-protobuf\r\n"
                                      "Content-length:";
    if(response.length() < muduo::detail::kSmallBuffer - 1024) {
        LogStream ss;
        ss << simple_resp << response.length() << "\r\n\r\n";
        ss << response;
        muduo::net::TcpConnectionPtr ptr = wptr_.lock();
        if(ptr) {
            MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
            ptr->send(ss.buffer().data(), ss.buffer().length());
        } else {
            //LOG_WARN << Id() << ":Connection Closed!";
            MON_ADD(ATTR_ADAPTER_TANX_RESPON_NOT_CONNECTD, 1);
        }
    } else {
        std::stringstream ss;
        ss << simple_resp << response.length() << "\r\n\r\n";
        ss << response;
        muduo::net::TcpConnectionPtr ptr = wptr_.lock();
        if(ptr) {
            MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
            const std::string &resp = ss.str();
            ptr->send(resp.c_str(), resp.length());
        } else {
            //LOG_WARN << Id() << ":Connection Closed!";
            MON_ADD(ATTR_ADAPTER_TANX_RESPON_NOT_CONNECTD, 1);
        }
    }
}

