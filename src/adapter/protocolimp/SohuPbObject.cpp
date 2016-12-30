#include "SohuPbObject.h"

#include <boost/algorithm/string.hpp>//to_lower
#include "../conf/Configer.h"
//#include <func.h>//to_str

extern poseidon::adapter::Configer *configer;//放在namespace里会链接错误

namespace poseidon
{
namespace adapter
{

using namespace muduo;
using namespace muduo::net;
using namespace sohuadx;
//using namespace rapidxml;


std::string SohuPbObject::dspid_;
rtb::TrafficSource SohuPbObject::source_ = rtb::TS_SOHU;
std::string SohuPbObject::click_url_;
std::string SohuPbObject::expose_url_;
std::string SohuPbObject::downloaded_url_;
std::string SohuPbObject::special_view_url_;
double SohuPbObject::sohu_resp_timeout_;
std::map<std::string, int> SohuPbObject::adtype_viewtype_map_;
//std::map<int, int> SohuPbObject::viewtype_adtype_map_;


namespace
{
//缓存request和response
DECLARE_THREAD_VAR_LIST(sohuadx::Request, sohu_req);
DECLARE_THREAD_VAR_LIST(sohuadx::Response, sohu_resp);
}

class InterClass
{
public:
    explicit InterClass(SohuPbObject *parent) : parent_(parent) {}
    ~InterClass();
private:
    SohuPbObject *parent_;
};


SohuPbObject::SohuPbObject() :
    request_(*Alloc_Bid_Obj(THREAD_VAR_LIST(sohu_req))),
    response_(*Alloc_Bid_Obj(THREAD_VAR_LIST(sohu_resp)))
{
    //ctor
    //srand(time(NULL));//randomize
}

SohuPbObject::~SohuPbObject()
{
    //dtor
    DestroyObject(&request_, THREAD_VAR_LIST(sohu_req));
    DestroyObject(&response_, THREAD_VAR_LIST(sohu_resp));
}

void SohuPbObject::OnThreadInitStatic()
{
    InitThreadedQueuePool(THREAD_VAR_LIST(sohu_req));
    InitThreadedQueuePool(THREAD_VAR_LIST(sohu_resp));
}

void SohuPbObject::InitStaticVar()
{
    dspid_ = configer->GetProperty<std::string>("base.sohu_dspid", "");
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.sohu_feedback_url", "");
    //sohu的点击，曝光url在审核时已经提供，此处不需要再给！
    //click_url_ = host + feedback + "/click?";
    //expose_url_ = host + feedback + "/feedback?";
    downloaded_url_ = host + feedback + "/download?";
    //special_view_url_ = host + feedback + "/view?";
    int timeout = configer->GetProperty<int>("net.sohu_response_timeout", 85);
    sohu_resp_timeout_ = static_cast<double>(timeout) / 1000;

    LOG_INFO << "sohu_resp_timeout:" << sohu_resp_timeout_;
    InitMap();
}

void SohuPbObject::InitMap()
{
    std::vector<std::pair<std::string, std::string>> viewtype_map =
                configer->GetSectionKeys("sohu_view_type_map");
    for (auto iter = viewtype_map.begin(); iter != viewtype_map.end(); ++iter) {
        adtype_viewtype_map_[iter->first] = atoi(iter->second.c_str());
        //viewtype_adtype_map_[atoi(iter->second.c_str())] = atoi(iter->first.c_str());
    }
    LOG_INFO << "sohu adtype_viewtype size:" << adtype_viewtype_map_.size();
}


void SohuPbObject::OnFailed()
{
    //失败返回200应答
    //InterClass final(this);
    //traceid_impid_map_.clear();
}

bool SohuPbObject::ParseFromBuff(const HttpRequest*, const char *buff, size_t len)
{
    return request_.ParseFromArray(buff, static_cast<int>(len));
}

//mutable_xxxx(string* value)
//string* mutable_xxx

int SohuPbObject::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    MON_ADD(ATTR_ADAPTER_SOHU_REQUEST, 1);
    //LOG_INFO << "[-->SohuPbObject Request<--]\n bid:" << Id();
    LOG_DEBUG << "[-->Sohu Request<--]\n" << request_.DebugString();

    rtb_request_->set_id(Id());
    response_.set_version(request_.version());
    response_.set_bidid(Id());
    if (request_.istest()) {
        return -1;
    }
    if (SetUser() != 0) {
        LOG_WARN << "SetUser failed!";
        return -1;
    }
    SetSite();
    if(SetDevice() != 0) {
        LOG_WARN << "SetDevice failed!";
        return -1;
    }
    if(SetImpression(rtb_request_list) != 0) {
        LOG_WARN << "SetImpression failed!";
        return -1;
    }

    MON_ADD(ATTR_ADAPTER_CONTROLER_SOHU_REQUEST, rtb_request_list.size());
    return 0;
}

void SohuPbObject::SetSite()
{
    auto *rtb_site = rtb_request_->mutable_site();
    auto &site = request_.site();
    //媒体名称, 目前可能取值为SOHU、SOHU_NEWS_APP、SOHU_WAP、SOHU_TV、SOHU_TV_APP、56
    site.has_name() ? rtb_site->set_name(site.name()) : void(0);
    site.has_page() ? rtb_site->set_page(site.page()) : void(0); //当前页面url
    //optional int64 category = 3;	//当前页面频道类别, newCMS分类
    site.has_ref() ? rtb_site->set_ref(site.ref()) : void(0);	//referrer url, 来源页面url
}

int SohuPbObject::SetUser()
{
    if(!request_.has_user()) {
        return 0;
    }
    rtb::User* pUser = rtb_request_->mutable_user();
    request_.user().has_suid() ? pUser->set_id(request_.user().suid()) : void(0);
    //repeated int64 category = 3;            //用户分类, 预留
    //repeated string searchKeyWords = 4;     //用户搜索词历史, 预留
    //optional string yyid = 5;               //yyid, 预留
    //optional string fyid = 6;               //fyid, 预留
    return 0;
}

int SohuPbObject::SetDevice()
{
    rtb::Device* pDevice = rtb_request_->mutable_device();
    auto &device = request_.device();
    pDevice->set_id(device.has_imei() ? device.imei() : device.idfa());
    device.has_ua() ? pDevice->set_user_agent(device.ua()) : void(0);
    device.has_ip() ? pDevice->set_ip(device.ip()) : void(0);

    std::string *p_carrier = request_.mutable_device()->mutable_carrier();
    if(p_carrier->length() > 0) {
        boost::to_lower(*p_carrier);
        if(*p_carrier == "china mobile") {
            pDevice->set_carrier(2);
        } else if(*p_carrier == "china unicom") {
            pDevice->set_carrier(3);
        } else if(*p_carrier == "china telecom") {
            pDevice->set_carrier(4);
        } else {
            pDevice->set_carrier(0);    //unknow
        }
    } else {
        pDevice->set_carrier(0);
    }

    auto *dev_type = request_.mutable_device()->mutable_type();
    boost::to_lower(*dev_type);
    if (*dev_type == "mobile") {
        pDevice->set_device_type(2);
    } else if (*dev_type == "pc") {
        pDevice->set_device_type(1);
    } else if (*dev_type == "wap") {
        pDevice->set_device_type(2);
    }

    auto *nettype = request_.mutable_device()->mutable_nettype();
    boost::to_lower(*nettype);
    if(*nettype == "4g") {
        pDevice->set_connection_type(static_cast<rtb::ConnectionType>(6));    //4G
    } else if(*nettype == "wifi") {
        pDevice->set_connection_type(static_cast<rtb::ConnectionType>(2));    //Wifi
    } else if(*nettype == "3g") {
        pDevice->set_connection_type(static_cast<rtb::ConnectionType>(5));    //3G
    } else if(*nettype == "2g") {
        pDevice->set_connection_type(static_cast<rtb::ConnectionType>(4));    //2G
    } else {
        pDevice->set_connection_type(static_cast<rtb::ConnectionType>(0));    //unknow
    }

    if (pDevice->device_type() == 2 && device.has_mobiletype()) {//mobile
        std::string *os = request_.mutable_device()->mutable_mobiletype();
        boost::to_lower(*os);
        if (*os == "androidphone" || *os == "androidpad") {
            pDevice->set_os("android");
        } else { //iPhone、iPad
            pDevice->set_os("ios");
        }
    }
    //optional uint32 screenWidth = 7;    //终端屏幕分辨率宽
    //optional uint32 screenHeight = 8;   //终端屏幕分辨率高
    //optional string imsi = 10;
    //optional string mac = 11;
    //optional string androidID = 13;
    //optional string openUDID = 14;
    //device.has_make() ? pDevice->set_make(device.make()) : void(0);
    //device.has_model() ? pDevice->set_model(device.model()) : void(0);
    //android_id
    return 0;
}

int SohuPbObject::SetImpression(std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    for (int i = 0; i < request_.impression_size(); ++i) {
        auto &imp = request_.impression(i);
        auto &pid = imp.pid();
        auto iter = adtype_viewtype_map_.find(pid);
        if (iter == adtype_viewtype_map_.end()) {
            LOG_WARN << "pid:" << pid << " has not maped view_type!";
            continue;
        }
        if (!rtb::ViewType_IsValid(iter->second)) {
            LOG_ERROR << "Invalid View_type value:" << iter->second;
            continue;
        }

        RtbReqSharedPtr rtb_request;
        //若单广告位，那么直接用预分配的值
        if (request_.impression_size() == 1) {
            rtb_request = rtb_request_;
        } else {
            rtb_request = Alloc_rtb_req_shared();
            rtb_request->CopyFrom(*rtb_request_);
        }
        rtb_request_list.push_back(rtb_request);
        rtb::Impression *pImp = rtb_request->add_impressions();
        pImp->mutable_ext()->set_view_type(iter->second);//兼容旧版本control
        pImp->add_view_types(iter->second);

        pImp->set_id(pid);
        pImp->set_bidfloor(imp.bidfloor());
        pImp->set_bidfloorcur(CurrencyCode[CHINA]);
        pImp->set_secure(NS_HTTP);
        pImp->mutable_ext()->set_ad_num(1);
        if (imp.has_banner()) {
            SetBanner(pImp, imp.banner());
        } else if (imp.has_video()) { //video
            SetVideo(pImp, imp.video());
        }

        if (imp.ispreferreddeals() && imp.tradingtype() == "PMP") {
            //SetImp_Pmp(imp.pmp(), pImp);
        }
        //acceptadvertisingtype //可接受的广告投放类型
        rtb_request->set_trace_id(CreateTraceId());
        traceid_impid_map_[rtb_request->trace_id()] = &request_.impression(i);
        LOG_DEBUG << "[<--Rtb Request-->]\n" << rtb_request->DebugString();
    }
    if(rtb_request_list.size() == 0) {
        return -1;
    }
    return 0;
}



void SohuPbObject::SetBanner(rtb::Impression *pImp, const Request_Impression_Banner &banner)
{
    rtb::Banner *rtb_banner = pImp->mutable_banner();
    rtb_banner->set_id("0");
    banner.has_width() ? rtb_banner->set_width(banner.width()) : void(0);
    banner.has_height() ? rtb_banner->set_height(banner.height()) : void(0);
    for (int i = 0; i < banner.mimes_size(); ++i) {
        if (banner.mimes(i) == 1) {//image
            rtb_banner->add_formats("image");
        }
        else if (banner.mimes(i) == 2) {//flash
            rtb_banner->add_formats("flash");
        }
        else if (banner.mimes(i) == 3) {//html
            rtb_banner->add_formats("html");
        }
    }
   //banner.template();
}

void SohuPbObject::SetVideo(rtb::Impression *pImp, const Request_Impression_Video &video)
{
    rtb::Video *pRtbVideo = pImp->mutable_video();
    for (int i = 0; i < video.mimes_size(); ++i) {
        switch (video.mimes(i)) {
        case 4:
            pRtbVideo->add_formats("video/mp4");
            break;
        default:
            pRtbVideo->add_formats("unknow");
            break;
        }

    }

    auto *rtb_content = pRtbVideo->mutable_ext()->mutable_content();
    video.has_durationlimit() ? pRtbVideo->set_max_duration(video.durationlimit()) : void(0);
    //pRtbVideo->set_protocol((rtb::VideoProtocol)video.protocol());
    video.has_width() ? pRtbVideo->set_width(video.width()) : void(0);
    video.has_height() ? pRtbVideo->set_height(video.height()) : void(0);
    video.has_pageurl() ? rtb_content->set_url(video.pageurl()) : void(0);
    for (int i = 0; i < video.category_size(); ++i) {//视频频道分类
        rtb_content->add_keywords(video.category(i));
    }
    video.has_title() ? rtb_content->set_title(video.title()) : void(0);
    auto *ext = rtb_content->mutable_ext();
    if (video.has_external()) {
        auto *dict = ext->add_direct();
        dict->set_key("external"); //扩展参数
        dict->set_value(video.external());
    }
    if (video.has_region()) {
        auto *dict = ext->add_direct();
        dict->set_key("region"); //视频产地分类
        dict->set_value(video.region());
    }
}

int SohuPbObject::RtbResponse(poseidon::rtb::BidResponse &rtb_response, EventLoop *loop)
{
    InterClass finalclass(this);
    MON_ADD(ATTR_ADAPTER_CONTROLER_SOHU_RESPONSE, 1);
    auto iter = traceid_impid_map_.find(rtb_response.trace_id());
    if (iter == traceid_impid_map_.end()) {
        return -1;    //impossible
    }
    auto *imp = iter->second;
    traceid_impid_map_.erase(iter);

    if (rtb_response.no_bid_reason() != 0) {
        return -1;
    }

    for (int i = 0; i < rtb_response.bid_seats_size(); ++i) {
        const rtb::BidSeat &rtb_bidseat = rtb_response.bid_seats(i);
        auto *seatbid = response_.add_seatbid();
        seatbid->set_idx(imp->idx());
        for (int j = 0; j < rtb_bidseat.bids_size(); ++j) {
            auto *pBid = seatbid->add_bid();
            const rtb::Bid &rtb_bid = rtb_bidseat.bids(j);
            SetResponse(rtb_response, pBid, rtb_bid, imp);
        }
    }
    return 0;
}

InterClass::~InterClass()
{
    if (parent_->traceid_impid_map_.size() == 0) {
        //只打印成功的出价日志
        if (parent_->response_.seatbid_size() > 0) {
            LOG_INFO << "[<--Sohu Response-->]\n" << parent_->response_.DebugString();
        }
        std::string resp;
        parent_->response_.SerializeToString(&resp);
        MON_ADD(ATTR_ADAPTER_SOHU_SUCC_RESPONSE, 1);
        parent_->AdxResponse(resp);
    }
}

void SohuPbObject::SetResponse(poseidon::rtb::BidResponse &rtb_response,
                               Response_Bid *pBid,
                               const rtb::Bid &rtb_bid,
                               const Request_Impression *imp)
{
    static const std::string price_macro = "s=%%WINPRICE%%";
    pBid->set_price(rtb_bid.price());
    //pBid->set_adurl();

    //构造监测地址
    std::string *expose_url = pBid->mutable_displaypara();//feedback
    std::string *click_url = pBid->mutable_clickpara();//click
    //std::string *winnotice = pBid->mutable_nurl();
    //审核串：http://fb.yousuode.cn:8080/adx/sohu/v2/mobile/feedback?%%DISPLAY%%
    BuildExpose(rtb_response, &rtb_bid, price_macro, *expose_url);
    //审核串：http://fb.yousuode.cn:8080/adx/sohu/v2/mobile/click?%%CLICK%%
    BuildClick(rtb_response, &rtb_bid, "", *click_url);
    //winnotice
    //BuildWinNotice(rtb_response, &rtb_bid, *winnotice);
    //build特殊曝光字符串
    //CustomBuildUrl(rtb_response, &rtb_bid, special_view_url_, "", 100, *special_expose);
}


//向Adx返回应答
void SohuPbObject::AdxResponse(const std::string &response)
{
    static const std::string simple_resp = "HTTP/1.1 200 OK\r\n"
                                           "Content-Type:application/x-protobuf\r\n"
                                           "Content-length:";
    LogStream ss;
    ss << simple_resp << response.length() << "\r\n\r\n";
    ss << response;
    muduo::net::TcpConnectionPtr ptr = wptr_.lock();
    if (ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(ss.buffer().data(), ss.buffer().length());
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_SOHU_RESPON_NOT_CONNECTD, 1);
    }
}

}//namespace adapter

}//namespace poseidon
