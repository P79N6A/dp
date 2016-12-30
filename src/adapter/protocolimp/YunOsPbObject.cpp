#include "YunOsPbObject.h"
#include <boost/algorithm/string.hpp>
#include <boost/pool/object_pool.hpp>
#include "utility/md5.h"
#include "utility/json.h"

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;
using namespace com::yunos::exchange::service::provider::dto;

extern Configer * volatile configer;
std::string YunOsPbObject::dspid_;
rtb::TrafficSource YunOsPbObject::source_ = rtb::TS_YUNOS;
std::string YunOsPbObject::click_url_;
std::string YunOsPbObject::expose_url_;
std::string YunOsPbObject::downloaded_url_;
std::string YunOsPbObject::yunos_secret_;
double YunOsPbObject::yunos_resp_timeout_;

namespace
{
DECLARE_THREAD_VAR_LIST(com::yunos::exchange::service::provider::dto::BidRequest, yunos_req);
DECLARE_THREAD_VAR_LIST(com::yunos::exchange::service::provider::dto::BidResponse, yunos_resp);
}

YunOsPbObject::YunOsPbObject() : send_empty_resp_(1),
    request_(*Alloc_Bid_Obj(THREAD_VAR_LIST(yunos_req))),
    response_(*Alloc_Bid_Obj(THREAD_VAR_LIST(yunos_resp)))
{
    //ctor
}

YunOsPbObject::~YunOsPbObject()
{
    //dtor
    //当整个对象需释放时，检查是否需要发送空应答！
    if(send_empty_resp_) {
        //LOG_INFO << "bid:" << Id() << " Destructor!";
        MON_ADD(ATTR_ADAPTER_YUNOS_EMPTY_RESPONSE, 1);
        com::yunos::exchange::service::provider::dto::BidResponse response;
        std::string empty_resp;
        response.set_bid(request_.bid());
        response.set_version(request_.version());
        //LOG_INFO << "[<--Yunos Empty Response-->]\n" << response.DebugString();
        response.SerializeToString(&empty_resp);
        Response(Compress(empty_resp));
    }
    DestroyObject(&request_, THREAD_VAR_LIST(yunos_req));
    DestroyObject(&response_, THREAD_VAR_LIST(yunos_resp));
}

void YunOsPbObject::OnThreadInitStatic()
{
    InitThreadedQueuePool(THREAD_VAR_LIST(yunos_req));
    InitThreadedQueuePool(THREAD_VAR_LIST(yunos_resp));
}

void YunOsPbObject::InitStaticVar()
{
    dspid_ = configer->GetProperty<std::string>("base.yunos_dspid", "");
    yunos_secret_ = configer->GetProperty<std::string>("token.yunos_secret", "");
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.yunos_feedback_url", "");
    //downloaded_url_ = host + feedback + "/download?";
    //click_url_ = host + feedback + "/click?";
    //expose_url_ = host + feedback + "/feedback?";

    int timeout = configer->GetProperty<int>("net.yunos_response_timeout", 40);
    //yunos平均rt：60ms
    yunos_resp_timeout_ = (double)timeout / 1000;
    LOG_INFO << "Yunos timeout:" << yunos_resp_timeout_;
}
//检查签名
int YunOsPbObject::AuthEntry()
{
    LogStream ls;
    ls << yunos_secret_;
    ls << "appName" << request_.appname();
    ls << "authKey" << request_.authkey();
    ls << "bid" << request_.bid();
    ls << "timestamp" << request_.timestamp();
    ls << "uuid" << request_.uuid();
    ls << "version" << request_.version();
    ls << yunos_secret_;

    //md5
    std::string str_md5 = md5(ls.buffer().data());
    //byte2hex
    //boost::to_upper(str_md5);
    //LOG_INFO << "MD5:" << str_md5 << " sign:" << request_.sign();
    return (str_md5 == request_.sign());
    //std::transform(str_md5.begin(), str_md5.end(), str_md5.begin(), ::to_up);
    //与request_.sign()对比
}

bool YunOsPbObject::ParseFromBuff(const HttpRequest *request, const char *buff, size_t len)
{
    //判断Content-encoding:zlib？  客户端请求一般带Accept-encoding
    static const std::string content_encode = "content-encoding";
    typedef std::map<std::string, std::string> Map;
    const Map& head_map = request->Headers();
    auto iter = head_map.find(content_encode);
    if(iter != head_map.end() && iter->second == "zlib") {
        char decod_buf[1024 * 64];
        size_t buf_len;
        if((buf_len = UnCompress(buff, len, decod_buf, sizeof(decod_buf))) > 0) {
            return request_.ParseFromArray(decod_buf, buf_len);
        }
    } else {
        return request_.ParseFromArray(buff, len);
    }
    return false;
}

int YunOsPbObject::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_req_list)
{
    MON_ADD(ATTR_ADAPTER_YUNOS_REQUEST, 1);
    //LOG_INFO << "[-->Yunos Request<--]\n bid:" << Id();
    //LOG_INFO << "[-->Yunos Request<--]\n" << request_.DebugString();
    //AuthEntry();
    rtb_request_->set_id(Id());
    //APP
    if(request_.has_appname()) {
        rtb::App *pApp = rtb_request_->mutable_app();
        pApp->set_bundle(request_.appname());
    }
    //Device
    rtb::Device* pDevice = rtb_request_->mutable_device();
    if(request_.has_screen()) {
        pDevice->mutable_ext()->set_dev_resolution(request_.screen());
    }
    const std::string &net = request_.network();
    if(net == "wifi") {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_WIFI);
    } else if(net == "4g") {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_4G);
    } else if(net == "3g") {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_3G);
    } else if(net == "2g") {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_2G);
    } else {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_UNKNOWN);
    }
    pDevice->set_os("yunos");
    //User
    rtb::User *user = rtb_request_->mutable_user();
    if(request_.has_uuid()) {
        user->set_id(request_.uuid());//用户id
        pDevice->set_id(request_.uuid());//uuid作为设备id。yunos手机的内部生成的唯一id~~和android无关
    }
    for(int i = 0; i < request_.usertag_size(); ++i) {
        user->add_keywords(request_.usertag(i));
    }

    //排除的package列表
    OpenDspJson::FastWriter json_writer;
    OpenDspJson::Value excluedpkgs;
    for(int i = 0; i < request_.excluedpkgs_size(); ++i) {
        excluedpkgs["excluedpkgs"].append(request_.excluedpkgs(i));
    }
    if(!excluedpkgs.empty()) {
        const std::string &exclude_str = json_writer.write(excluedpkgs);
        rtb_request_->mutable_ext()->set_request_json(exclude_str);
    }

    //有些特殊字段，在rtb请求无对应的，则用json传递
    //OpenDspJson::Value impressions;
    for(int i = 0; i < request_.ads_size(); ++i) {
        RtbReqSharedPtr rtb_req;
        if (request_.ads_size() == 1) {
            rtb_req = rtb_request_;
        } else {
            rtb_req = Alloc_rtb_req_shared();
            rtb_req->CopyFrom(*rtb_request_);
        }
        rtb_req_list.push_back(rtb_req);


        OpenDspJson::Value impression;
        const BidRequest_AD &adinfo = request_.ads(i);
        rtb::Impression *pImp = rtb_req->add_impressions();
        pImp->set_id(adinfo.adid());
        pImp->mutable_ext()->set_view_type(rtb::VT_YUNOS_APP_AD);
        pImp->mutable_ext()->set_ad_num(adinfo.bidcount());

        //format to json
        for(int j = 0; j < adinfo.categories_size(); ++j) {
            impression["categories"].append(adinfo.categories(j));
        }
        for(int j = 0; j < adinfo.pkgs_size(); ++j) {
            impression["pkgs"].append(adinfo.pkgs(j));
        }
        if(adinfo.keyword().length() > 0) {
            impression["keyword"] = adinfo.keyword();
        }

        impression["postype"] = adinfo.postype();
        //impressions["ad"].append(impression);
        const std::string &ad_str = json_writer.write(impression);
        pImp->mutable_ext()->set_special_json(ad_str);
        rtb_req->set_trace_id(CreateTraceId());
        adid_list_.insert(std::make_pair(rtb_req->trace_id(), adinfo.adid()));
        LOG_DEBUG << "[<--Rtb Request-->]\n" << rtb_req->DebugString();
    }
    MON_ADD(ATTR_ADAPTER_CONTROLER_YUNOS_REQUEST, rtb_req_list.size());
    return 0;
}

void YunOsPbObject::OnFailed()
{
    //LOG_INFO << "OnFailed, bid:" << Id();
    //build a empty body
    //do nothing. destructor procdure send empty resp
}

//每个广告会返回一次！
int YunOsPbObject::RtbResponse(rtb::BidResponse &rtb_response, EventLoop *loop)
{
    MON_ADD(ATTR_ADAPTER_CONTROLER_YUNOS_RESPONSE, 1);
    RtbResponFinal respon_guard(*this, rtb_response);

    auto iter = adid_list_.find(rtb_response.trace_id());
    if(iter == adid_list_.end()) {
        return -1;    //impossible!!
    }
    const std::string &impid = iter->second;
    if(response_.bid().length() == 0) {
        response_.set_bid(request_.bid());
    }
    if(!response_.has_version()) {
        response_.set_version(request_.version());
    }

    if(rtb_response.no_bid_reason() != 0) { //不出价！
        BidResponse_AD *ad = response_.add_ads();
        ad->set_adid(impid);
        return 0;
    }

    for(int i = 0; i < rtb_response.bid_seats_size(); ++i) {
        BidResponse_AD *ad = response_.add_ads();
        //ad->set_adid(request_.ads(i).adid());
        for(int j = 0; j < rtb_response.bid_seats(i).bids_size(); ++j) { //bidCount
            const rtb::Bid &bid = rtb_response.bid_seats(i).bids(j);
            ad->set_adid(impid);
            BidResponse_App *app = ad->add_apps();
            app->set_price(bid.price());
            //std::string final_url;
            //std::string click_url = "u=";
            //click_url += bid.ext().dest_url();
            //BuildClick(&bid, click_url, final_url);
            app->set_downloadurl(bid.ext().dest_url());

            const std::string &json = bid.specific_data();
            OpenDspJson::Value specific;
            if(!OpenDspJson::Reader().parse(json, specific)) {
                LOG_ERROR << "非法的Yunos json返回值！";
                return -1;
            }
            SURE_CVALUE(app->set_packagename, specific["packagename"]);
            SURE_CVALUE(app->set_signature, specific["signature"]);
            SURE_CVALUE(app->set_md5, specific["md5"]);
            app->set_versioncode(ASINT(specific["versioncode"]));
            app->set_size(ASINT(specific["size"]));

            std::string expose_str;
            BuildExpose(rtb_response, &bid, "", expose_str);
            app->set_external(expose_str);
        }
    }

    return 0;
}


YunOsPbObject::RtbResponFinal::~RtbResponFinal()
{
    parent_.adid_list_.erase(rtb_resp_.trace_id());
    //Yunos RTB Response. bid:8f3f2917838c11e6bb3700163e0438f5 impid:2 adid_list.size:0
    //LOG_INFO << "Yunos RtbResponse. bid:" << rtb_resp_.id() <<
    //            " adid_list.size:" << parent_.adid_list_.size();
    if(parent_.adid_list_.size() == 0) { //已经全部返回
        if (parent_.response_.ads_size() > 0) {
            LOG_INFO << "[<--Yunos Response-->]\n" << parent_.response_.DebugString();
        }
        std::string response_str;
        parent_.response_.SerializeToString(&response_str);
        MON_ADD(ATTR_ADAPTER_YUNOS_SUCC_RESPONSE, 1);
        parent_.send_empty_resp_ = 0;//不再发送空应答
        parent_.Response(parent_.Compress(response_str));
    }
}



//向Adx返回应答
void YunOsPbObject::Response(const std::string &response)
{
    static const string simple_resp = "HTTP/1.1 200 OK\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      "Content-Type:application/x-protobuf\r\n"
                                      "Content-encoding:zlib\r\n"
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
            MON_ADD(ATTR_ADAPTER_YUNOS_RESPON_NOT_CONNECTD, 1);
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
            MON_ADD(ATTR_ADAPTER_YUNOS_RESPON_NOT_CONNECTD, 1);
        }
    }
}

