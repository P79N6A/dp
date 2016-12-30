#include "IQiYiPBObject.h"
//#include <func.h>//to_str
#include <boost/algorithm/string.hpp>
#include "net/EventLoopContext.h"
#include "net/RedisClient.h"
#include "utility/rapidxml.hpp"
#include "utility/rapidxml_print.hpp"

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;
using namespace ads_serving::proto;
using namespace rapidxml;

extern Configer * volatile configer;

std::string IQiYiPBObject::dspid_;
rtb::TrafficSource IQiYiPBObject::source_ = rtb::TS_IQIYI;
std::string IQiYiPBObject::click_url_;
std::string IQiYiPBObject::expose_url_;
std::string IQiYiPBObject::downloaded_url_;
double IQiYiPBObject::iqiyi_resp_timeout_;
std::map<int64_t, std::vector<int> > IQiYiPBObject::pid_viewtype_map_;

namespace
{
DECLARE_THREAD_VAR_LIST(ads_serving::proto::BidRequest, iqiyi_req);
DECLARE_THREAD_VAR_LIST(ads_serving::proto::BidResponse, iqiyi_resp);
}



IQiYiPBObject::IQiYiPBObject() : empty_resp_(1),
    controler_resp_flag_(1),
    iqiyi_request_(*Alloc_Bid_Obj(THREAD_VAR_LIST(iqiyi_req))),
    response_(*Alloc_Bid_Obj(THREAD_VAR_LIST(iqiyi_resp)))
{
    //ctor
}

IQiYiPBObject::~IQiYiPBObject()
{
    //dtor
    if(empty_resp_) {
        //LOG_INFO << "[<--Iqiyi empty Response-->] bid:" << Id();
        BuildEmptyResp();
    }
    DestroyObject(&iqiyi_request_, THREAD_VAR_LIST(iqiyi_req));
    DestroyObject(&response_, THREAD_VAR_LIST(iqiyi_resp));
}

void IQiYiPBObject::OnThreadInitStatic()
{
    InitThreadedQueuePool(THREAD_VAR_LIST(iqiyi_req));
    InitThreadedQueuePool(THREAD_VAR_LIST(iqiyi_resp));
}

void IQiYiPBObject::InitStaticVar()
{
    dspid_ = configer->GetProperty<std::string>("base.iqiyi_dspid", "");
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.iqiyi_feedback_url", "");
    click_url_ = host + feedback + "/click?";
    expose_url_ = host + feedback + "/feedback?";
    downloaded_url_ = host + feedback + "/download?";

    int timeout = configer->GetProperty<int>("net.iqiyi_response_timeout", 120);
    iqiyi_resp_timeout_ = static_cast<double>(timeout) / 1000;

    LOG_INFO << "iqiyi_resp_timeout:" << iqiyi_resp_timeout_;
    ViewMap();
}

void IQiYiPBObject::ViewMap()
{
    //key-value
    std::vector<std::pair<std::string, std::string>> viewtype_map =
                configer->GetSectionKeys("iqiyi_view_type_map");

    for(size_t i = 0; i < viewtype_map.size(); ++i) {
        std::vector<std::string> list;
        std::pair<std::string, std::string> &pair = viewtype_map[i];
        std::string &view_type_list = pair.second;
        int64_t impid = atoll(pair.first.c_str());
        pid_viewtype_map_[impid].clear();//add a new impid slot
        boost::split(list, view_type_list, boost::is_any_of(";"));
        for(size_t j = 0; j < list.size(); ++j) {
            pid_viewtype_map_[impid].push_back(atoi(list[j].c_str()));
        }
    }
}



void IQiYiPBObject::OnFailed()
{
    //do nothing
}

bool IQiYiPBObject::ParseFromBuff(const HttpRequest*, const char *buff, size_t len)
{
    return iqiyi_request_.ParseFromArray(buff, static_cast<int>(len));
}

int IQiYiPBObject::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    MON_ADD(ATTR_ADAPTER_IQIYI_REQUEST, 1);
    //LOG_INFO << "[-->Iqiyi Request<--]\n bid:" << Id();
    LOG_DEBUG << "[-->Iqiyi Request<--]\n" << iqiyi_request_.DebugString();
    if(iqiyi_request_.is_ping()) {
        return 0;
    }
    rtb_request_->set_id(Id());
    if(SetUser() != 0) {
        LOG_WARN << "SetUser failed!";
        return -1;
    }
    if(SetSite() != 0) {
        LOG_WARN << "SetSite failed!";
        return -1;
    }
    if(SetDevice() != 0) {
        LOG_WARN << "SetDevice failed!";
        return -1;
    }
    if(SetImpression(rtb_request_list) != 0) {
        LOG_WARN << "SetImpression failed!";
        return -1;
    }
    MON_ADD(ATTR_ADAPTER_CONTROLER_IQIYI_REQUEST, rtb_request_list.size());
    return 0;
}

int IQiYiPBObject::SetUser()
{
    if(!iqiyi_request_.has_user()) {
        return 0;
    }
    rtb::User* pUser = rtb_request_->mutable_user();
    const std::string &user_id = iqiyi_request_.user().id();//device_id
    pUser->set_id(user_id);
    return 0;
}

int IQiYiPBObject::SetSite()
{
    if(!iqiyi_request_.has_site()) {
        return 0;
    }
    rtb::Site *pSite = rtb_request_->mutable_site();
    unsigned int site_id = iqiyi_request_.site().id();
    char buff[32];
    snprintf(buff, sizeof(buff), "%u", site_id);
    pSite->set_site_id(buff);
    /*
    if(iqiyi_request_.site().has_content())
    {
        const Content &content = iqiyi_request_.site().content();
        rtb::App *app = rtb_request_->mutable_app();
        for(int i = 0; i < content.keyword_size(); ++i)
        {
            app->add_keywords(content.keyword(i));
        }
    }
    */
    return 0;
}

void IQiYiPBObject::SetContent(rtb::Content *pContent, const /*site::*/Content &content)
{
    content.has_title() ? pContent->set_title(content.title()) : void(0);
    content.has_url() ? pContent->set_url(content.url()) : void(0);
    for(int i = 0; i < content.keyword_size(); ++i) {
        pContent->add_keywords(content.keyword(i));
    }
    //视频时长
    content.has_len() ? pContent->set_len(content.len()) : void(0);
    //剧目信息
    if(content.has_album_id()) {
        auto *d1 = pContent->mutable_ext()->add_direct();
        d1->set_key("s");
        char buff[32];
        snprintf(buff, sizeof(buff), "%lld", content.album_id());
        d1->set_value(buff);
    }

    //1	电影
    //2	电视剧
    //3	纪录片
    //... ...
    if(content.has_channel_id()) {
        auto *d2 = pContent->mutable_ext()->add_direct();
        d2->set_key("channel");
        char buff[32];
        snprintf(buff, sizeof(buff), "%lld", content.channel_id());
        d2->set_value(buff);
    }
}

int IQiYiPBObject::SetDevice()
{
    rtb::Device* pDevice = rtb_request_->mutable_device();
    const Device &device = iqiyi_request_.device();
    device.has_ua() ? pDevice->set_user_agent(device.ua()) : void(0);
    device.has_ip() ? pDevice->set_ip(device.ip()) : void(0);
    int connectiontype = device.connection_type();
    // 0：未知; 1：以太网2：Wifi; 3：2G; 4:3G; 5:4G
    if(connectiontype == 2) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_WIFI);
    } else if(connectiontype == 1) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_ETHERNET);
    } else if(connectiontype == 5) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_4G);
    } else if(connectiontype == 4) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_3G);
    } else if(connectiontype == 3) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_2G);
    } else if(connectiontype == 0) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_UNKNOWN);
    }

    int platform_id = device.platform_id();
    switch(platform_id) {
    case 32://iPhone客户端
    case 35://win手机客户端
    case 33://Android手机端
    case 311://Android手机H5
    case 312://iPhoneH5（贴片不可点击）
        pDevice->set_device_type(DT_PHONE);
        break;
    case 21://IPad H5（贴片可点击）
    case 22://iPad客户端
    case 23://Android平板客户端
    case 25://win平板客户端
        pDevice->set_device_type(DT_TABLET);
        break;
    case 11://PC网页端
    case 12://PC客户端
    case 13://Mac客户端
        pDevice->set_device_type(DT_PC);
        break;
    case 5201:
        pDevice->set_device_type(DT_TV);
        break;
    default://unknow
        LOG_ERROR << "Invalid platform_id!";
        return -1;
    }

    std::string os;
    switch(platform_id) { //os
    case 23://Android平板客户端
    case 33://Android手机端
    case 311://Android手机H5
    case 5201:
        os = "android";
        break;
    case 21://IPad H5（贴片可点击）
    case 22://iPad客户端
    case 32://iPhone客户端
    case 321://iPhoneH5（贴片不可点击）
        os = "ios";
        break;
    case 11://PC网页端
    case 12://PC客户端
    case 25://win平板客户端
        os = "windows";
        break;
    case 13://Mac客户端
        os = "mac";
        break;
    default:
        return -1;
    }
    //v2.5 add:
    device.has_os() ? pDevice->set_os(device.os()) : pDevice->set_os(os);
    device.has_os_version() ? pDevice->set_os_ver(device.os_version()) : (void)0;
    device.has_model() ? pDevice->set_model(device.model()) : (void)0;

    if(iqiyi_request_.has_user()) {
        const std::string &dev_id = iqiyi_request_.user().id();
        pDevice->set_id(dev_id);//device_id属于加密信息
    }
    pDevice->set_carrier(1);//unknown

    //由于各exchange使用的geo信息类型不同，此处先不转换
    //pDevice->mutable_geo()->
    return 0;
}


int IQiYiPBObject::SetImpression(std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    for(int i = 0; i < iqiyi_request_.imp_size(); ++i) {
        const Impression &imp = iqiyi_request_.imp(i);
        //先检查ad_zone_id是否属于已映射
        int64_t ad_zone_id;
        if(imp.has_banner()) {
            ad_zone_id = imp.banner().ad_zone_id();
        } else if(imp.has_video()) {
            ad_zone_id = imp.video().ad_zone_id();
        } else {
            return -1;
        }
        auto iter = pid_viewtype_map_.find(ad_zone_id);
        if (iter == pid_viewtype_map_.end()) {
            LOG_WARN << "unknow impid:" << ad_zone_id;
            continue;
        }

        RtbReqSharedPtr rtb_request;
        if (iqiyi_request_.imp_size() == 1) {
            rtb_request = rtb_request_;
        } else {
            rtb_request = Alloc_rtb_req_shared();
            rtb_request->CopyFrom(*rtb_request_);
        }
        rtb_request_list.push_back(rtb_request);
        //add_impressions后不能有return返回，以免发生空广告位给control
        rtb::Impression *pImp = rtb_request->add_impressions();

        char buff[32];
        snprintf(buff, sizeof(buff), "%lld", ad_zone_id);
        pImp->set_id(buff);

        /*
        //v2.5 add:
        //不同行业设置不同底价。当在 floor_price 找不
        //到所需要行业的底价的时候，就用 bidfloor 作
        //为当前的流量的底价
        //100000000	游戏
        //200000000	电商
        //300000000	其他
        //400000000	品牌
        //500000000	网服与APP
        //600000000	金融行业
        //iqiyi回复：如果都是游戏，其实判断不用那么复杂，贴片就俩价格，K类城市72，A、B类是4元
        */
        //bidfloor是这个广告位的底板价，即所有行业最低的那个
        int price = imp.bidfloor();
        for (int j = 0; j < imp.floor_price_size(); ++j) {
            auto &iter = imp.floor_price(j);
            if (iter.industry() == 100000000LL) {
                price = iter.price();
                break;
            }
        }
        pImp->set_bidfloor(price);
        pImp->set_bidfloorcur(CurrencyCode[CHINA]);
        pImp->set_secure(NS_HTTP);
        pImp->mutable_ext()->set_ad_num(1);
        if(imp.has_banner()) {
            SetBanner(pImp, imp.banner());
        } else if(imp.has_video()) {
            std::vector<int> &view_type_list = iter->second;
            if (ad_zone_id == 1000000000381LL) { //贴片
                int ad_type = imp.video().ad_type();
                if(ad_type == 1) {
                    pImp->mutable_ext()->set_view_type(rtb::VT_IQY_GEN_LINEAR_0);    //前帖
                } else if(ad_type == 2) {
                    pImp->mutable_ext()->set_view_type(rtb::VT_IQY_GEN_LINEAR_1);    //中
                } else if(ad_type == 3) {
                    pImp->mutable_ext()->set_view_type(rtb::VT_IQY_GEN_LINEAR_2);    //后
                }
            } else if (view_type_list.size() > 0 && rtb::ViewType_IsValid(view_type_list[0])) {
                pImp->mutable_ext()->set_view_type(static_cast<rtb::ViewType>(view_type_list[0]));
            } else {
                LOG_ERROR << "bid:" << Id() << " has no view_type!";
            }

            SetVideo(pImp, imp.video());
        }
        //imp.campaign_id  campaign id可以理解为deal  id
        //imp.blocked_ad_tag
        //imp.blocked_ad_attribute
        if(imp.has_is_pmp() && imp.is_pmp()) {
            rtb::PMP *pmp = pImp->mutable_pmp();
            auto *deal = pmp->add_deals();
            char deal_id[32];
            snprintf(deal_id, sizeof(deal_id), "%d", imp.campaign_id());
            deal->set_id(deal_id);
        }

        rtb_request->set_trace_id(CreateTraceId());
        traceid_impid_map_[rtb_request->trace_id()] = &(iqiyi_request_.imp(i));
        LOG_DEBUG << "[<--Rtb Request-->]\n" << rtb_request->DebugString();
    }
    if(rtb_request_list.size() == 0) {
        return -1;
    }
    return 0;
}

void IQiYiPBObject::SetBanner(rtb::Impression *pImp, const Banner &banner)
{
    char buff[32];
    snprintf(buff, sizeof(buff), "%lld", banner.ad_zone_id());
    pImp->mutable_banner()->set_id(buff);
}

void IQiYiPBObject::SetVideo(rtb::Impression *pImp, const Video &video)
{
    rtb::Video *pRtbVideo = pImp->mutable_video();
    //linearity iqiyi:
    //1. Linear. example: pre-roll, mid-roll and post-roll.
    //2. Non-linear. example: overlay, video link, pause, and tool bar.

    //linearity RTB:
    //0: "未知";
    //1："instream/linear"即线性贴片素材，包括前贴中贴后贴;
    //2:"overlay/nonlinear"即视频播放中的悬浮广告;
    //3："pause"即视频播放暂停中的悬浮广告;
    //4:"fullscreen"即视频全屏播放时的悬浮广告;
    if(rtb::VideoLinearity_IsValid(video.linearity())) {
        pRtbVideo->set_linearity(static_cast<rtb::VideoLinearity>(video.linearity()));
    }

    int ad_type = video.ad_type();//0页面, 1 pre-rool, 2 mid-roll, 3 post-roll,
    // 4 corner, 5 video link, 6 pause, 7 tool bar, 8相关商品, 9 overlay.
    switch(ad_type) {
    case 1://前帖
        pRtbVideo->set_start_delay(static_cast<poseidon::rtb::VideoStartDelay>(0));
        break;
    case 2://中贴
        pRtbVideo->set_start_delay(static_cast<poseidon::rtb::VideoStartDelay>(1));
        break;
    case 3://后贴
        pRtbVideo->set_start_delay(static_cast<poseidon::rtb::VideoStartDelay>(2));
        break;
    //set_linearity:
    case 4:
    case 5:
    case 7:
    case 9:
        pRtbVideo->set_linearity(rtb::VIDEO_LINEARITY_NON_LINEAR);
        break;
    case 6:
        pRtbVideo->set_linearity(rtb::VIDEO_LINEARITY_PAUSE);
        break;
    default:
        pRtbVideo->set_linearity(rtb::VIDEO_LINEARITY_UNKNOW);
        break;
    }
    // set min_duration 视频广告最短播放时长，单位是秒
    video.has_minduration() ? pRtbVideo->set_min_duration(video.minduration()) : void(0);
    video.has_maxduration() ? pRtbVideo->set_max_duration(video.maxduration()) : void(0);
    switch(video.protocol()) {
    case 1:
        pRtbVideo->set_protocol(rtb::VIDEO_PROTOCOL_VAST_10);
        break;
    case 2:
        pRtbVideo->set_protocol(rtb::VIDEO_PROTOCOL_VAST_20);
        break;
    case 3:
        pRtbVideo->set_protocol(rtb::VIDEO_PROTOCOL_VAST_30);
        break;
    case 4:
    case 5:
    case 6:
        pRtbVideo->set_protocol(rtb::VIDEO_PROTOCOL_VAST_WRAPPER);
        break;
    default:
        LOG_WARN << "invalid VAST protocol! bid:" << Id();
    }
    video.has_w() ? pRtbVideo->set_width(video.w()) : void(0);
    video.has_h() ? pRtbVideo->set_height(video.h()) : void(0);
    video.has_startdelay() ? pRtbVideo->mutable_ext()->set_section_start_delay(video.startdelay()) : void(0);
    //video.is_entire_roll is not match!

    if(iqiyi_request_.site().has_content()) {
        rtb::Content* pRtbVideoContent = pRtbVideo->mutable_ext()->mutable_content();
        const Content &content = iqiyi_request_.site().content();
        SetContent(pRtbVideoContent, content);
    }
}

int IQiYiPBObject::RtbResponse(poseidon::rtb::BidResponse &rtb_response, EventLoop *loop)
{
    InterClass finalclass(this);
    MON_ADD(ATTR_ADAPTER_CONTROLER_IQIYI_RESPONSE, 1);
    auto iter = traceid_impid_map_.find(rtb_response.trace_id());
    if(iter == traceid_impid_map_.end()) {
        return -1;    //impossible
    }
    const Impression *imp = iter->second;
    traceid_impid_map_.erase(iter);

    if(rtb_response.no_bid_reason() != 0) {
        //Seatbid *iqy_seatbid = response_.add_seatbid();
        return -1;
    }

    controler_resp_flag_ <<= 1;
    (!response_.has_id()) ? response_.set_id(Id()) : void(0);//请求ID
    for(int i = 0; i < rtb_response.bid_seats_size(); ++i) {
        const rtb::BidSeat &rtb_bidseat = rtb_response.bid_seats(i);
        Seatbid *iqy_seatbid = response_.add_seatbid();
        for(int j = 0; j < rtb_bidseat.bids_size(); ++j) {
            Bid *iqy_pBid = iqy_seatbid->add_bid();
            const rtb::Bid &rtb_bid = rtb_bidseat.bids(j);
            SetResponse(rtb_response, iqy_pBid, rtb_bid, imp);
        }
    }
    return 0;
}

IQiYiPBObject::InterClass::~InterClass()
{
    //traceid_impid_map_.size() == 0表示全部广告位已经从controler返回。
    //controler_resp_flag_ > 1表示至少有一个广告位是成功出价的
    if(parent_->traceid_impid_map_.size() == 0 && parent_->controler_resp_flag_ > 1) {
        std::string resp;
        LOG_INFO << "[<--Iqiyi Response-->]\n" << parent_->response_.DebugString();
        parent_->response_.SerializeToString(&resp);
        MON_ADD(ATTR_ADAPTER_IQIYI_SUCC_RESPONSE, 1);
        parent_->empty_resp_ = 0;
        parent_->AdxResponse(resp);
    }
}


void IQiYiPBObject::SetResponse(poseidon::rtb::BidResponse &rtb_response,
                                Bid *iqy_pBid,
                                const rtb::Bid &rtb_bid,
                                const Impression *imp)
{
    iqy_pBid->set_id(rtb_bid.id());
    // ID of the impression object to which this bid applies.
    // 非ad_zone_id
    iqy_pBid->set_impid(imp->id());
    iqy_pBid->set_price(rtb_bid.price());
    iqy_pBid->set_crid(rtb_bid.ext_cid());
    if(imp->has_video()) { //视频类型
        VideoResp(rtb_response, iqy_pBid, rtb_bid, imp);
    } else {
        LOG_WARN << "Banner? but now is not support!";
    }
}

void IQiYiPBObject::VideoResp(poseidon::rtb::BidResponse &rtb_response,
                              ads_serving::proto::Bid *iqy_bid,
                              const rtb::Bid &rtb_bid,
                              const ads_serving::proto::Impression *imp)
{
    static const std::string price_macro = "s=${SETTLEMENT}";
    LogStream ls;
    //Build a VAST Response
    ls << "<VAST version=\"3.0\">";
    ls << "<Ad><InLine><AdSystem>xxxx.com</AdSystem><AdTitle/><Impression>";
    std::string strfeedback;
    BuildExpose(rtb_response, &rtb_bid, price_macro, strfeedback);
    ls << "<![CDATA[" << strfeedback << "]]></Impression>";
    ls << "<Creatives><Creative>";
    //20171108确认：全走liner，不走NonLinearAds了
    //if(imp->video().linearity() == 1)//视频贴片 or etc
    {
        ls << "<Linear>";
        ls << "<VideoClicks>";
        //type属性，只支持移动端。
        //0.默认类型，点击后通过内置 WebView 打开落地页
        //11.APP 直接下载
        //4.点击APP下载，点击后弹出对话框询问用户是否下载 APP
        ls << "<ClickThrough type = \"11\">";
        ls << "<![CDATA[" << rtb_bid.ext().dest_url() << "]]></ClickThrough>";
        ls << "<ClickTracking>";
        std::string click_trace_url;
        BuildClick(rtb_response, &rtb_bid, "", click_trace_url);
        ls << "<![CDATA[" << click_trace_url << "]]></ClickTracking>";
        ls << "</VideoClicks>";
        ls << "<Icons><Icon><StaticResource><![CDATA[" <<
           configer->GetProperty<std::string>("base.aligame_logo_url", "");
        ls << "]]></StaticResource></Icon></Icons>";
        ls << "</Linear>";
    }

    //test
    /*
    TcpConnectionPtr conn = wptr_.lock();
    if (conn) {
        EventLoop *loop = conn->getLoop();
        LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());
        LogStream cmd;
        cmd << "set ";
        cmd << rtb_response.trace_id() << " ";
        cmd << "zhangxianghui";
        loop_ctx->redis_client->Command(cmd.buffer().data(), cmd.buffer().length());
    }
    */
    /*
    //Non-linear. example: overlay, video link, pause, and tool bar.
    else if(imp->video().linearity() == 2)
    {
        ls << "<NonLinearAds>";
        ls << "<NonLinear height=\"" << rtb_bid.h() << "\" width=\"" << rtb_bid.w() << "\">";
        ls << "<StaticResource creativeType=\"image/jpeg\">";
        ls << "<NonLinearClickThrough><![CDATA[" << rtb_bid.ext().dest_url() << "]]></NonLinearClickThrough>";
        std::string click_trace;
        BuildClick(rtb_response, &rtb_bid, "", click_trace);
        ls << "<NonLinearClickTracking><![CDATA[" << click_trace << "]]></NonLinearClickTracking>";
        ls << "</StaticResource>";
        ls << "</NonLinear>";

        ls << "<NonLinear height=\"25\" width=\"25\">";
        ls << configer->GetProperty<std::string>("base.aligame_logo_url", "");
        ls << "</NonLinear>";

        ls << "</NonLinearAds>";
    }
    */
    ls << "</Creative>";
    ls << "</Creatives>";
    ls << "</InLine>";
    ls << "</Ad>";
    ls << "</VAST>";
    iqy_bid->set_adm(ls.buffer().data(), ls.buffer().length());
}

/*
void IQiYiPBObject::VideoResp(poseidon::rtb::BidResponse &rtb_response,
                              ads_serving::proto::Bid *iqy_bid,
                              const rtb::Bid &rtb_bid,
                              const ads_serving::proto::Impression *imp)
{
    static const std::string price_macro = "s=${SETTLEMENT}";
    //VAST
    xml_document<> doc;
    xml_node<>* rot = doc.allocate_node(rapidxml::node_element, "VAST");
    rot->append_attribute(doc.allocate_attribute("version","3.0"));
    doc.append_node(rot);

    xml_node<>* ad = doc.allocate_node(node_element,"Ad");
    rot->append_node(ad);

    xml_node<>* in_line = doc.allocate_node(node_element,"InLine");
    ad->append_node(in_line);

    in_line->append_node(doc.allocate_node(node_element,"AdSystem","xxxx.com"));
    in_line->append_node(doc.allocate_node(node_element,"AdTitle"));
    xml_node<>* impression = doc.allocate_node(node_element,"Impression");
    in_line->append_node(impression);
    //impression add cdata
    std::string strfeedback;
    BuildExpose(rtb_response, &rtb_bid, price_macro, strfeedback);
    impression->append_node(doc.allocate_node(node_cdata, 0,
                                              strfeedback.c_str(), 0, strfeedback.length()));

    xml_node<>* creatives = doc.allocate_node(node_element,"Creatives");
    in_line->append_node(creatives);

    xml_node<>* creative = doc.allocate_node(node_element,"Creative");
    creatives->append_node(creative);
    if(imp->video().linearity() == 1)//视频贴片 or etc
    {
        xml_node<>* linear_ads = doc.allocate_node(node_element,"Linear");
        creative->append_node(linear_ads);

        xml_node<>* video_click = doc.allocate_node(node_element,"VideoClicks");
        linear_ads->append_node(video_click);
        xml_node<>* clickthrough = doc.allocate_node(node_element,"ClickThrough");
        //type属性，只支持移动端。
        //0.默认类型，点击后通过内置 WebView 打开落地页
        //11.APP 直接下载
        //4.点击APP下载，点击后弹出对话框询问用户是否下载 APP
        clickthrough->append_attribute(doc.allocate_attribute("type", "4"));
        //点击跳转，即落地页地址
        clickthrough->append_node(doc.allocate_node(node_cdata, 0,
                                                    rtb_bid.ext().dest_url().c_str(),
                                                    0));
        video_click->append_node(clickthrough);

        std::string click_trace_url;
        BuildClick(rtb_response, &rtb_bid, "", click_trace_url);
        xml_node<>* click_tracking = doc.allocate_node(node_element, "ClickTracking", click_trace_url.c_str());
        //点击监测
        //click_tracking->append_node(doc.allocate_node(node_cdata, 0,
        //                                              click_trace_url.c_str(),
        //                                              0));
        video_click->append_node(click_tracking);

        xml_node<>* icons = doc.allocate_node(node_element,"Icons");
        linear_ads->append_node(icons);

        xml_node<>* duration = doc.allocate_node(node_element,"Duration");//TODO

    }
    //Non-linear. example: overlay, video link, pause, and tool bar.
    //
    else if(imp->video().linearity() == 2)
    {
        xml_node<>* nonlinear_ads = doc.allocate_node(node_element,"NonLinearAds");
        creative->append_node(nonlinear_ads);

        //非线性标签，暂停页流量必选标签，必选属性：width、height，可选属性：apiFramework、id
        xml_node<>* nonlinear = doc.allocate_node(node_element,"NonLinear");
        char buff[64];
        snprintf(buff, sizeof(buff), "%d", rtb_bid.h());
        nonlinear->append_attribute(doc.allocate_attribute("height", buff));

        snprintf(buff, sizeof(buff), "%d", rtb_bid.w());
        nonlinear->append_attribute(doc.allocate_attribute("width", buff));
        nonlinear_ads->append_node(nonlinear);


        //素材资源标签，必选属性：type；资源url。当前支持type取值：
        //image/jpeg、image/png、application/x-shockwave-flash
        xml_node<>* staticResource = doc.allocate_node(node_element, "StaticResource");
        int creative_format = rtb_bid.ext().creative_format();
        //TODO:judge creative_format!!
        staticResource->append_attribute(doc.allocate_attribute("creativeType", "image/jpeg"));
        //staticResource add cdata
        staticResource->append_node(doc.allocate_node(node_cdata, 0, rtb_bid.image_url().c_str(),
                                                      0, rtb_bid.image_url().length()));
        nonlinear->append_node(staticResource);

        //素材点击跳转地址标签
        xml_node<>* nonLinearClickThrough = doc.allocate_node(node_element, "NonLinearClickThrough");
        //nonLinearClickThrough add cdata
        nonLinearClickThrough->append_node(doc.allocate_node(node_cdata, 0,
                                                             rtb_bid.ext().dest_url().c_str(), 0,
                                                             rtb_bid.ext().dest_url().length()));
        nonlinear->append_node(nonLinearClickThrough);

        std::string click_trace;
        BuildClick(rtb_response, &rtb_bid, "", click_trace);
        xml_node<>* nonLinearclicktracking = doc.allocate_node(node_element,"NonLinearClickTracking");
        nonLinearclicktracking->append_node(doc.allocate_node(node_cdata, 0,
                                                             click_trace.c_str(), 0,
                                                             click_trace.length()));

        nonlinear->append_node(nonLinearclicktracking);
    }
    std::string *vast_xml = iqy_bid->mutable_adm();
    vast_xml->clear();
    rapidxml::print(std::back_inserter(*vast_xml), doc, 0);
}
*/

//向adx发送一个空应答
void IQiYiPBObject::BuildEmptyResp()
{
    static const string simple_resp = "HTTP/1.1 204 No Content\r\n\r\n";
    MON_ADD(ATTR_ADAPTER_IQIYI_EMPTY_RESPONSE, 1);

    //LOG_INFO << "OUT-id_youtu_empty_resp:" << Id();
    muduo::net::TcpConnectionPtr ptr = wptr_.lock();
    if(ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(simple_resp.c_str(), static_cast<int>(simple_resp.length()));
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_IQIYI_RESPON_NOT_CONNECTD, 1);
    }
}

//向Adx返回应答
void IQiYiPBObject::AdxResponse(const std::string &response)
{
    static const string simple_resp = "HTTP/1.1 200 OK\r\n"
                                      "Content-Type:application/x-protobuf\r\n"
                                      "Content-length:";
    LogStream ss;//logstream大小目前设为20KB，因此不要返回大于20KB的数据。
    ss << simple_resp << response.length() << "\r\n\r\n";
    ss << response;
    muduo::net::TcpConnectionPtr ptr = wptr_.lock();
    if(ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(ss.buffer().data(), ss.buffer().length());
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_IQIYI_RESPON_NOT_CONNECTD, 1);
    }
}

