#include "TodayToutiao.h"

#include <queue>
#include <boost/algorithm/string.hpp>
#include <boost/pool/object_pool.hpp>
//#include <func.h>//to_str
#include "utility/json.h"//解析bid.specifadata字段

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;
using namespace toutiao_ssp::api;
//using namespace rapidxml;

extern Configer * volatile configer;

std::string TodayToutiao::dspid_;
rtb::TrafficSource TodayToutiao::source_ = rtb::TS_TOUTIAO;
std::string TodayToutiao::click_url_;
std::string TodayToutiao::expose_url_;
std::string TodayToutiao::downloaded_url_;
std::string TodayToutiao::special_view_url_;
double TodayToutiao::toutiao_resp_timeout_;
std::map<int, int> TodayToutiao::adtype_viewtype_map_;
std::map<int, int> TodayToutiao::viewtype_adtype_map_;


namespace
{
//主要是为了减少new操作
DECLARE_THREAD_VAR_LIST(toutiao_ssp::api::BidRequest, tt_req);
DECLARE_THREAD_VAR_LIST(toutiao_ssp::api::BidResponse, tt_resp);
}


TodayToutiao::TodayToutiao() : empty_resp_(1),
    request_(*Alloc_Bid_Obj(THREAD_VAR_LIST(tt_req))),
    response_(*Alloc_Bid_Obj(THREAD_VAR_LIST(tt_resp)))
{
    //ctor
    //srand(time(NULL));//randomize
}

TodayToutiao::~TodayToutiao()
{
    //dtor
    if(empty_resp_) {
        //LOG_INFO << "[<--Todaytoutiao empty Response-->] bid:" << Id();
        BuildEmptyResp();
    }

    DestroyObject(&request_, THREAD_VAR_LIST(tt_req));
    DestroyObject(&response_, THREAD_VAR_LIST(tt_resp));
}

void TodayToutiao::OnThreadInitStatic()
{
    InitThreadedQueuePool(THREAD_VAR_LIST(tt_req));
    InitThreadedQueuePool(THREAD_VAR_LIST(tt_resp));
}

void TodayToutiao::InitStaticVar()
{
    dspid_ = configer->GetProperty<std::string>("base.todaytoutiao_dspid", "");
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.todaytoutiao_feedback_url", "");
    click_url_ = host + feedback + "/click?";
    expose_url_ = host + feedback + "/feedback?";
    downloaded_url_ = host + feedback + "/download?";
    special_view_url_ = host + feedback + "/view?";
    int timeout = configer->GetProperty<int>("net.todaytoutiao_response_timeout", 120);
    toutiao_resp_timeout_ = static_cast<double>(timeout) / 1000;

    LOG_INFO << "todaytoutiao_resp_timeout:" << toutiao_resp_timeout_;
    InitMap();
}

void TodayToutiao::InitMap()
{
    std::vector<std::pair<std::string, std::string>> viewtype_map =
                configer->GetSectionKeys("todaytoutiao_view_type_map");
    for (auto iter = viewtype_map.begin(); iter != viewtype_map.end(); ++iter) {
        adtype_viewtype_map_[atoi(iter->first.c_str())] = atoi(iter->second.c_str());
        viewtype_adtype_map_[atoi(iter->second.c_str())] = atoi(iter->first.c_str());
    }
    LOG_INFO << "todaytoutiao adtype_viewtype size:" << adtype_viewtype_map_.size();
}


void TodayToutiao::OnFailed()
{
    //失败返回200应答
    //InterClass final(this);
    //traceid_impid_map_.clear();
}

bool TodayToutiao::ParseFromBuff(const HttpRequest*, const char *buff, size_t len)
{
    return request_.ParseFromArray(buff, static_cast<int>(len));
}

//mutable_xxxx(string* value)
//string* mutable_xxx

int TodayToutiao::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    MON_ADD(ATTR_ADAPTER_TOUTIAO_REQUEST, 1);
    //LOG_INFO << "[-->Todaytoutiao Request<--]\n bid:" << Id();
    LOG_DEBUG << "[-->Todaytoutiao Request<--]\n" << request_.DebugString();
    //请求类型，1 为只请求直投广告，2 为只请求 DSP 广告，
    //3 为同时请求两种广告，默认为 2
    if(request_.bid_req_type() == 1) {
        LOG_ERROR << "not support req_type=1";
        return -1;//目前暂不支持直投广告！
    }
    rtb_request_->set_id(Id());
    if(SetUser() != 0) {
        LOG_WARN << "SetUser failed!";
        return -1;
    }
    if(SetApp() != 0) {
        LOG_WARN << "SetApp failed!";
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
    MON_ADD(ATTR_ADAPTER_CONTROLER_TOUTIAO_REQUEST, rtb_request_list.size());
    return 0;
}

int TodayToutiao::SetUser()
{
    if(!request_.has_user()) {
        return 0;
    }
    rtb::User* pUser = rtb_request_->mutable_user();
    request_.user().has_id() ? pUser->set_id(request_.user().id()) : void(0);
    request_.user().has_buyer_id() ? pUser->set_buyer_id(request_.user().buyer_id()) : void(0);
    //yob年龄
    request_.user().has_yob() ? pUser->set_year_of_birth(atoi(request_.user().yob().c_str())) : void(0);
    if(request_.user().gender() == User_Gender_MALE) {
        pUser->set_gender(rtb::GENDER_MALE);
    } else if(request_.user().gender() == User_Gender_FEMALE) {
        pUser->set_gender(rtb::GENDER_FEMALE);
    } else if(request_.user().gender() == User_Gender_UNKNOWN) {
        pUser->set_gender(rtb::GENDER_UNKNOWN);
    }

    if (request_.user().keywords().length() > 0) {
        std::vector<std::string> list;
        boost::split(list, request_.user().keywords(), boost::is_any_of(","));
        for(size_t i = 0; i < list.size(); ++i) {
            list[i].length() > 0 ? pUser->add_keywords(list[i]) : void(0);
        }
    }

    if(request_.user().has_geo()) {
        const Geo &geo = request_.user().geo();
        pUser->mutable_geo()->set_lat(geo.lat());
        pUser->mutable_geo()->set_lon(geo.lon());
        pUser->mutable_geo()->set_country(geo.country());
        pUser->mutable_geo()->set_region(geo.region());
        pUser->mutable_geo()->set_city(geo.city());
        pUser->mutable_geo()->set_type(atoi(geo.type().c_str()));
    }

    for(int i = 0; i < request_.user().data_size(); ++i) {
        const Data &data = request_.user().data(i);
        rtb::Data *pData = pUser->add_data();
        data.has_id() ? pData->set_id(data.id()) : void(0);
        data.has_name() ? pData->set_name(data.name()) : void(0);
        for(int j = 0; j < data.segment_size(); ++j) {
            rtb::Segment *segment = pData->add_segments();
            auto &seg = data.segment(j);
            seg.has_id() ? segment->set_id(seg.id()) : void(0);
            seg.has_name() ? segment->set_name(seg.name()) : void(0);
            seg.has_value() ? segment->set_value(seg.value()) : void(0);
        }
    }
    return 0;
}

int TodayToutiao::SetApp()
{
    if(!request_.has_app()) {
        return 0;
    }
    const App &app = request_.app();
    rtb::App *pApp = rtb_request_->mutable_app();
    app.has_id() ? pApp->set_id(app.id()) : void(0);
    app.has_name() ? pApp->set_name(app.name()) : void(0);
    app.has_domain() ? pApp->set_domain(app.domain()) : void(0);
    app.has_ver() ? pApp->set_version(app.ver()) : void(0);
    app.has_bundle() ? pApp->set_bundle(app.bundle()) : void(0);
    app.has_privacypolicy() ? pApp->set_privacy_policy(app.privacypolicy()) : void(0);
    app.has_paid() ? pApp->set_paid(app.paid()) : void(0);

    const Publisher &pub = app.publisher();
    rtb::Publisher *rtb_pub = pApp->mutable_publisher();
    pub.has_id() ? rtb_pub->set_id(pub.id()) : void(0);
    pub.has_name() ? rtb_pub->set_name(pub.name()) : void(0);
    pub.has_cat() ? rtb_pub->add_categories(atoi(pub.cat().c_str())) : void(0);
    pub.has_domain() ? rtb_pub->set_domain(pub.domain()) : void(0);

    rtb::Content *rtb_content = pApp->mutable_content();
    const Content &content = app.content();
    SetContent(rtb_content, content);

    if (app.keywords().length() > 0) {
        std::vector<std::string> list;
        boost::split(list, app.keywords(), boost::is_any_of(","));
        for(size_t i = 0; i < list.size(); ++i) {
            list[i].length() > 0 ? pApp->add_keywords(list[i]) : void(0);
        }
    }
    return 0;
}

void TodayToutiao::SetContent(rtb::Content *pContent, const Content &content)
{
    content.has_id() ? pContent->set_id(content.id()) : void(0);
    content.has_title() ? pContent->set_title(content.title()) : void(0);
    content.has_series() ? pContent->set_series(content.series()) : void(0);
    content.has_url() ? pContent->set_url(content.url()) : void(0);
    if (content.keywords().length() > 0) {
        std::vector<std::string> list;
        boost::split(list, content.keywords(), boost::is_any_of(","));
        for(size_t i = 0; i < list.size(); ++i) {
            list[i].length() > 0 ? pContent->add_keywords(list[i]) : void(0);
        }
    }
    content.has_contentrating() ? pContent->set_content_rating(content.contentrating()) : void(0);
    content.has_userrating() ? pContent->set_user_rating(content.userrating()) : void(0);
    //pContent->set_context(content.context());TODO
    rtb::Producer *rtb_producer = pContent->mutable_producer();
    const Content_Producer &producer = content.producer();
    producer.has_id() ? rtb_producer->set_id(producer.id()) : void(0);
    producer.has_name() ? rtb_producer->set_name(producer.name()) : void(0);
    producer.has_cat() ? rtb_producer->add_categories(atoi(producer.cat().c_str())) : void(0);
    producer.has_domain() ? rtb_producer->set_domain(producer.domain()) : void(0);
    content.has_language() ? pContent->set_language(content.language()) : void(0);
}

int TodayToutiao::SetDevice()
{
    rtb::Device* pDevice = rtb_request_->mutable_device();
    const Device &device = request_.device();
    pDevice->set_id(device.device_id());
    pDevice->set_do_not_track(device.dnt() ? 0 : 1);
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

    device.has_make() ? pDevice->set_make(device.make()) : void(0);
    device.has_model() ? pDevice->set_model(device.model()) : void(0);
    //os转换成小写
    std::string *os = request_.mutable_device()->mutable_os();
    if(os->length() > 0) {
        boost::to_lower(*os);
        pDevice->set_os(device.os());
    }
    device.has_osv() ? pDevice->set_os_ver(device.osv()) : void(0);
    device.has_js() ? pDevice->set_js(device.js()) : void(0);

    pDevice->set_device_type((device.device_type() == Device_DeviceType_PHONE) ? 2 : 3);

    if(device.has_geo()) {
        const Geo &geo = device.geo();
        rtb::Geo *rtb_geo = pDevice->mutable_geo();
        geo.has_lat() ? rtb_geo->set_lat(geo.lat()) : (void)0;
        geo.has_lon() ? rtb_geo->set_lon(geo.lon()) : (void)0;
        geo.has_country() ? rtb_geo->set_country(geo.country()) : (void)0;
        geo.has_region() ? rtb_geo->set_region(geo.region()) : (void)0;
        geo.has_city() ? rtb_geo->set_city(geo.city()) : (void)0;
        geo.has_type() ? rtb_geo->set_type(atoi(geo.type().c_str())) : (void)0;
    }

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
    //android_id
    return 0;
}

int TodayToutiao::SetImpression(std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    //头条只有一个广告位，段子有三个
    for (int i = 0; i < request_.adslots_size(); ++i) {
        const AdSlot &imp = request_.adslots(i);
        //从多个ad_type中随机挑选一个有效的ad_type
        //目前必须下载的ad_type才能抛包下载。
        std::vector<int> viewtypes;
        for (int j = 0; j < imp.ad_type_size(); ++j) {
            AdType ad_type = imp.ad_type(j);
            auto iter = adtype_viewtype_map_.find(ad_type);
            if (iter == adtype_viewtype_map_.end()) {
                LOG_WARN << "ad_type:" << ad_type << " has not maped view_type!";
                continue;
            }
            if (!rtb::ViewType_IsValid(iter->second)) {
                LOG_ERROR << "Invalid View_type value:" << iter->second;
                continue;
            }
            viewtypes.push_back(iter->second);//多view_type支持
        }
        if (viewtypes.size() == 0) {
            continue;    //本广告位的所有ad_type均无映射到相应的view_type
        }

        RtbReqSharedPtr rtb_request;
        if (request_.adslots_size() == 1) {
            rtb_request = rtb_request_;
        } else {
            rtb_request = Alloc_rtb_req_shared();
            rtb_request->CopyFrom(*rtb_request_);
        }
        rtb_request_list.push_back(rtb_request);
        rtb::Impression *pImp = rtb_request->add_impressions();
        //banner.pos指定了开屏，信息流（段子），详情页三种类型
        pImp->mutable_ext()->set_view_type(viewtypes[0]);//兼容旧版本control
        for (size_t k = 0; k < viewtypes.size(); ++k) {
            pImp->add_view_types(viewtypes[k]);
        }
        pImp->set_id("tt1");
        pImp->set_bidfloor(imp.bid_floor());
        pImp->set_bidfloorcur(CurrencyCode[CHINA]);
        pImp->set_secure(NS_HTTP);
        pImp->mutable_ext()->set_ad_num(1);

        char buff[32];
        rtb::Content* pRtbVideoContent =  pImp->mutable_video()->mutable_ext()->mutable_content();
        if (imp.has_channel_id()) {
            //channel全部放在video::content::ext::direct中
            auto *d2 = pRtbVideoContent->mutable_ext()->add_direct();
            d2->set_key("channel");
            snprintf(buff, sizeof(buff), "%llu", imp.channel_id());
            d2->set_value(buff);
        }
        if (imp.has_timestamp()) {
            auto *d1 = pRtbVideoContent->mutable_ext()->add_direct();
            d1->set_key("timestamp");
            snprintf(buff, sizeof(buff), "%llu", imp.timestamp());
            d1->set_value(buff);
        }

        if (imp.banner_size() > 0) {
            SetBanner(pImp, imp.banner(0));
        } else if (imp.has_patch_video_length()) { //video  没啥用，信息太少
            rtb::Video *pRtbVideo = pImp->mutable_video();
            pRtbVideo->set_max_duration(imp.patch_video_length());
        }

        if (imp.has_pmp()) {
            SetImp_Pmp(imp.pmp(), pImp);
        }

        rtb_request->set_trace_id(CreateTraceId());
        traceid_impid_map_[rtb_request->trace_id()] = &request_.adslots(i);
        LOG_DEBUG << "[<--Rtb Request-->]\n" << rtb_request->DebugString();
    }
    if(rtb_request_list.size() == 0) {
        return -1;
    }
    return 0;
}

void TodayToutiao::SetImp_Pmp(const Pmp &pmp, rtb::Impression *pImp)
{
    rtb::PMP *pPmp = pImp->mutable_pmp();
    pPmp->set_private_auction(PA_SPECIFIC_DEALS);
    for (int i = 0; i < pmp.deals_size(); ++i) {
        const Pmp_Deal &deal = pmp.deals(i);
        rtb::PMP_Deal *pDeal= pPmp->add_deals();
        char buff[32];
        snprintf(buff, sizeof(buff), "%d", deal.id());
        pDeal->set_id(buff);//deal.id值为 dspid
        pDeal->set_bidfloorcur(CurrencyCode[CHINA]);
        //deal 价格,单位是分/千次曝光,即CPM
        pDeal->set_bidfloor(deal.bid_floor());
        for (int i = 0; i < deal.wseat_size(); ++i) {
            pDeal->add_wseat(deal.wseat(i));
        }
        for (int i = 0; i < deal.wadomain_size(); ++i) {
            pDeal->add_wadomain(deal.wadomain(i));
        }
        // 竞拍类型
        // 1 -- first price auction.
        // 2 -- second price auction.
        // 3 -- the passed bidfloor indicates the apriori agreed upon deal price
        //竞价的方式，目前都是1，即第一竞价法。最高的deal获得竞价成功，取最高出价作为最终胜出价。
        //注：只有当多个Deal同时响应时才互相之间竞价。这个字段和OpenRTB相同。
        deal.has_at() ? pDeal->set_at(deal.at()) : void(0);
    }
}

void TodayToutiao::SetBanner(rtb::Impression *pImp, const AdSlot_Banner &banner)
{
    rtb::Banner *rtb_banner = pImp->mutable_banner();
    rtb_banner->set_id("0");
    banner.has_width() ? rtb_banner->set_width(banner.width()) : void(0);
    banner.has_height() ? rtb_banner->set_height(banner.height()) : void(0);
    /*
    SPLASH=1//开屏
    FEED=2//信息流
    DETAIL=4//详情页
    头条的dsp比较奇怪，不同类型有不同dspid
    */
    int pos = banner.pos();
    switch (pos) {
    case 1://SPLASH=1//开屏
        rtb_banner->set_position(8);//自定义8
        rtb_request_->mutable_ext()->set_dsp_id("1756163168");//开屏
        break;
    case 2://FEED=2//信息流
        rtb_banner->set_position(9);//自定义9
        rtb_request_->mutable_ext()->set_dsp_id("1756163164");//头条推荐流和段子推荐流
        break;
    case 3:
        rtb_banner->set_position(10);//自定义10
        rtb_request_->mutable_ext()->set_dsp_id("1756163174");//未知。。
        break;
    case 4://DETAIL=4//详情页
        rtb_banner->set_position(11);//自定义11
        rtb_request_->mutable_ext()->set_dsp_id("1756163162");//详情页
        break;
    default:
        rtb_banner->set_position(0);//unknow
        break;
    }
}

int TodayToutiao::RtbResponse(poseidon::rtb::BidResponse &rtb_response, EventLoop *loop)
{
    InterClass finalclass(this);
    MON_ADD(ATTR_ADAPTER_CONTROLER_TOUTIAO_RESPONSE, 1);
    auto iter = traceid_impid_map_.find(rtb_response.trace_id());
    if (iter == traceid_impid_map_.end()) {
        return -1;    //impossible
    }
    const AdSlot *imp = iter->second;
    traceid_impid_map_.erase(iter);

    if (rtb_response.no_bid_reason() != 0) {
        //build an empty struct
        //SeatBid *seatbid = response_.add_seatbids();
        //Bid *bid = seatbid->add_ads();
        //bid->set_adslot_id(imp->id());
        return -1;
    }

    for (int i = 0; i < rtb_response.bid_seats_size(); ++i) {
        const rtb::BidSeat &rtb_bidseat = rtb_response.bid_seats(i);
        SeatBid *seatbid = response_.add_seatbids();
        for (int j = 0; j < rtb_bidseat.bids_size(); ++j) {
            Bid *pBid = seatbid->add_ads();
            const rtb::Bid &rtb_bid = rtb_bidseat.bids(j);
            SetResponse(rtb_response, pBid, rtb_bid, imp);
        }
    }
    return 0;
}

TodayToutiao::InterClass::~InterClass()
{
    if (parent_->traceid_impid_map_.size() == 0) {

        if (!(parent_->response_.has_request_id())) {
            parent_->response_.set_request_id(parent_->request_.request_id());    //请求ID
        }
        //只打印成功的出价日志
        if (parent_->response_.seatbids_size() > 0) {
            LOG_INFO << "[<--Todaytoutiao Response-->]\n" << parent_->response_.DebugString();
        }
        std::string resp;
        parent_->response_.SerializeToString(&resp);
        //char output[]
        //io::ZeroCopyOutputStream
        //parent_->response_.SerializeToZeroCopyStream
        parent_->empty_resp_ = 0;
        MON_ADD(ATTR_ADAPTER_TOUTIAO_SUCC_RESPONSE, 1);
        parent_->AdxResponse(resp);
    }
}

void TodayToutiao::SetResponse(poseidon::rtb::BidResponse &rtb_response,
                               Bid *pBid,
                               const rtb::Bid &rtb_bid,
                               const AdSlot *imp)
{
    static const std::string price_macro = "s={bid_price}";
    //pBid->set_id(rtb_bid.id());
    pBid->set_id(rtb_response.trace_id() + rtb_bid.id());//头条那边要求长一点的字符串，先改为trace_id试试
    // ID of the impression object to which this bid applies.
    // 非ad_zone_id
    pBid->set_adslot_id(imp->id());
    pBid->set_price(rtb_bid.price());
    pBid->set_adid(atoll(rtb_bid.creative_id().c_str()));//不能填rtb_bid.ad_id

    MaterialMeta *mtm = pBid->mutable_creative();
    auto ad_type = viewtype_adtype_map_[rtb_bid.view_type()];
    if (toutiao_ssp::api::AdType_IsValid(ad_type)) {
        mtm->set_ad_type((AdType)ad_type);
    }
    //title:应用下载广告和 detail 一 banner 类型广告可不返回
    if (rtb_bid.ext().has_title() &&
            !ADTYPE_APP_DOWNLOAD(ad_type) &&
            !ADTYPE_DETAIL_BANNER(ad_type)) {
        //12.06与瑞坤商量，统一使用ad_words返回
        //mtm->mutable_title()->swap(*rtb_bid.mutable_ext()->mutable_title());//标题一般不超过25字
        mtm->set_title(rtb_bid.ext().ad_words());//标题一般不超过25字
    }
    //打开落地页有两种方式。true:外部浏览器打开。false：app内置webview打开。默认true
    mtm->set_is_inapp(true);

    //解析包名和应用名称
    const std::string &json = rtb_bid.specific_data();
    OpenDspJson::Value specific;
    if (!OpenDspJson::Reader().parse(json, specific)) {
        LOG_ERROR << "非法的bid.specific_data json返回值！";
        //return;
    }
    std::string package_name;
    std::string app_name;
    int bid_ad_type = -1;
    if (!specific["packagename"].empty()) {
        package_name = specific["packagename"].asString();
    }
    if (!specific["appname"].empty()) { //应用名称长度限定6个
        app_name = specific["appname"].asString();
    }

    if (imp->banner_size() > 0) {
        MaterialMeta_ImageMeta *image = mtm->mutable_image_banner();
        image->set_width(rtb_bid.w());
        image->set_height(rtb_bid.h());
        //如果是应用下载广告必须返回描述
        if (ADTYPE_APP_DOWNLOAD(ad_type)) {
            const std::string &desc = rtb_bid.ext().has_ad_words() ?
                                      rtb_bid.ext().ad_words() : app_name;
            image->set_description(desc);//此字段显示在广告图片上边的文字行中
        }
        //图片链接 url，若为组图或开屏联播，选urls 中任意一个图片地址即可，不能为空
        image->set_url(rtb_bid.image_url());
        //组图(ad_type=11)的 3 个图片链接，开屏联播(ad_type=13)的 5 个图片链接，有序
        auto &urls = specific["urls"];
        if (ADTYPE_HAS_URLS(ad_type) && !urls.empty() &&
                urls.type() == OpenDspJson::arrayValue) {
            for (OpenDspJson::Value::ArrayIndex i = 0; i < urls.size(); ++i) {
                const std::string &val = urls[i].asString();
                val.length() > 0 ? image->add_urls(val) : void(0);
            }
        }
        //组图test
        //image->add_urls("http://sh.image.xxxx.cn/s/y9l/s/material/ee8b347d9eacc5af035b51b18eda2011.jpg");
        //image->add_urls("http://sh.image.xxxx.cn/s/y9l/s/material/1e696bfb4681930dc5a7a9cd27f939eb.jpg");
        //image->add_urls("http://sh.image.xxxx.cn/s/y9l/s/material/40dc587d89aee5b313ad4ca8383932a5.jpg");
    }
    //source长度限定10个。开屏广告不填   // 来源, 默认dsp名
    if (!ADTYPE_SPLASH(ad_type)) {
        //经测试，下载类传这个回去没用。展现广告图片下的文字是用送审的标题
        app_name.length() > 0 ? mtm->set_source(app_name.substr(0, 10)) : void(0);//应用名称。
    }

    const std::string &dst_url = rtb_bid.ext().dest_url();
    //信息流落地页广告、详情页图文、视频广告、开屏广告为必须返回
    //外链对象，信息流应用下载，详情页banner 可不返回
    //q:开屏广告的点击地址可以是应用下载地址吗？a:开屏联播只能投落地页广告，落地页 url 在 external 字段中
    if (!ADTYPE_APP_DOWNLOAD(ad_type)) { //判断非下载才需要填落地页广告。
        MaterialMeta_ExternalMeta *external = mtm->mutable_external();
        external->set_url(dst_url);//落地页
    }

    if (ADTYPE_APP_DOWNLOAD(ad_type) && request_.device().os() == "android") {
        MaterialMeta_AndroidApp *aa = mtm->mutable_android_app();//App 名称
        aa->set_app_name(app_name.substr(0, 6));//应用名称长度限定6个
        //aa->set_web_url();//应用的详情页链接
        aa->set_download_url(dst_url);//应用下载链接
        //aa->set_open_url();//安装后打开应用的链接
        package_name.length() > 0 ? aa->set_package(package_name) : void(0);//应用包名，用于过滤已安装用户
    } else if (ADTYPE_APP_DOWNLOAD(ad_type) && request_.device().os() == "ios") {
        MaterialMeta_IosApp *ios_app = mtm->mutable_ios_app();
        ios_app->set_app_name(app_name.substr(0, 6));
        ios_app->set_download_url(dst_url);
        //ios_app->set_open_url();
        //ios_app->set_ipa_url();
        //ios_app->set_Appleid();
    }
    //构造监测地址
    std::string *special_expose = mtm->add_show_url();//view
    std::string *click_url = mtm->add_click_url();//click
    //由于报表结算是根据曝光(action=1)来的，故此处把feebackurl
    //传给竞价胜出监测
    std::string *expose_url = mtm->mutable_nurl();//feedback
    BuildExpose(rtb_response, &rtb_bid, price_macro, *expose_url);
    BuildClick(rtb_response, &rtb_bid, "", *click_url);
    //build特殊曝光字符串
    CustomBuildUrl(rtb_response, &rtb_bid, special_view_url_, "", 100, *special_expose);

    //test
    /*
    TcpConnectionPtr conn = wptr_.lock();
    EventLoop *loop = conn->getLoop();
    LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());
    LogStream cmd;
    cmd << "set ";
    cmd << rtb_response.trace_id() << " ";
    cmd << expose_url;
    loop_ctx->redis_client->Command(cmd.buffer().data(), cmd.buffer().length());
    */
}


//向adx发送一个空应答
void TodayToutiao::BuildEmptyResp()
{
    static const std::string simple_resp = "HTTP/1.1 204 No Content\r\n\r\n";
    MON_ADD(ATTR_ADAPTER_TOUTIAO_EMPTY_RESPONSE, 1);

    //LOG_INFO << "OUT-id_Todaytoutiao_empty_resp:" << Id();
    muduo::net::TcpConnectionPtr ptr = wptr_.lock();
    if (ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(simple_resp.c_str(), static_cast<int>(simple_resp.length()));
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_TOUTIAO_RESPON_NOT_CONNECTD, 1);
    }
}

//向Adx返回应答
void TodayToutiao::AdxResponse(const std::string &response)
{
    static const std::string simple_resp = "HTTP/1.1 200 OK\r\n"
                                           "Content-Type:application/x-protobuf\r\n"
                                           "Content-length:";
    LogStream ss;//不要返回大于20KB的数据。
    ss << simple_resp << response.length() << "\r\n\r\n";
    ss << response;
    muduo::net::TcpConnectionPtr ptr = wptr_.lock();
    if (ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(ss.buffer().data(), ss.buffer().length());
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_TOUTIAO_RESPON_NOT_CONNECTD, 1);
    }
}
