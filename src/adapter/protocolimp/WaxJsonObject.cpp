#include "WaxJsonObject.h"
#include <vector>

#include "utility/StringCodec.h"
#include <boost/algorithm/string.hpp>

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;
using namespace OpenDspJson;


extern Configer * volatile configer;


rtb::TrafficSource WaxJsonObject::source_ = rtb::TS_WAX;
std::string WaxJsonObject::dspid_;
std::string WaxJsonObject::winnotice_url_;
std::string WaxJsonObject::click_url_;
std::string WaxJsonObject::expose_url_;
std::string WaxJsonObject::downloaded_url_;
double WaxJsonObject::wax_resp_timeout_;

WaxJsonObject::WaxJsonObject() : empty_response_flag_(0)
{
    //ctor
    /*
    srand(recv_time_);
    view_type_vec_.push_back(rtb::VT_WAX_FEED_NOR);
    view_type_vec_.push_back(rtb::VT_WAX_FEED_ACTIVITY);
    view_type_vec_.push_back(rtb::VT_WAX_FEED_VIDEO);
    view_type_vec_.push_back(rtb::VT_WAX_FEED_GRID);
    */
}

WaxJsonObject::~WaxJsonObject()
{
    //dtor
    if (empty_response_flag_ > 0) {
        //写两次，可改为一次写出
        LOG_INFO << "[<--Wax Response-->]\n" << OpenDspJson::StyledWriter().write(response_);
        OpenDspJson::FastWriter writer;
        const std::string &json_str = writer.write(response_);
        WaxSuccResponse(json_str);
    } else {//若请求是{}空消息头，那么Id()会raise exception!
        //LOG_INFO << "[<--Wax Empty Response-->] bid:" << Id();
        WaxFailedResponse();
    }
}

void WaxJsonObject::OnThreadInitStatic()
{

}

void WaxJsonObject::InitStaticVar()
{
    dspid_ = configer->GetProperty<std::string>("base.wax_dspid", "");//配置的waxdspid
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.wax_feedback_url", "");
    click_url_ = host + feedback + "/click?";
    expose_url_ = host + feedback + "/feedback?";
    downloaded_url_ = host + feedback + "/download?";
    winnotice_url_ = host + feedback + "/winnotice?";

    int timeout = configer->GetProperty<int>("net.wax_response_timeout", 80);
    wax_resp_timeout_ = (double)timeout / 1000;
    LOG_INFO << "wax_resp_timeout:" << wax_resp_timeout_;
}


bool WaxJsonObject::ParseFromBuff(const HttpRequest*, const char *buff, size_t len)
{
    return reader_.parse(buff, buff + len, root_);
}

//当adxRequest处理失败时，进行的回调
void WaxJsonObject::OnFailed()
{
    // WaxFailedResponse(wptr_);
}


int WaxJsonObject::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    LOG_DEBUG << "[-->Wax Request<--]\n" << OpenDspJson::StyledWriter().write(root_);
    MON_ADD(ATTR_ADAPTER_WAX_REQUEST, 1);
    const OpenDspJson::Value &imp = root_["imp"];
    const OpenDspJson::Value &app = root_["app"];
    const OpenDspJson::Value &device = root_["device"];
    const OpenDspJson::Value &user = root_["user"];
    //root_["rule"] //TODO
    if (imp.empty() || device.empty() || user.empty()) {
        LOG_ERROR << "Invalid Wax Json object!";
        return -1;
    }

    if (imp.type() != OpenDspJson::arrayValue) {
        LOG_ERROR << "imp must be a array type!";
        return -3;
    }

    rtb_request_->set_id(Id());
    int ret;

    try { //jsoncpp在转换值错误时，会抛出异常。因此用try...excption包裹
        ret = SetApp(app);
        if (ret != 0) {
            //LOG_WARN << "SetApp failed!";
            return -1;
        }

        ret = SetDevice(device);
        if (ret != 0) {
            //LOG_WARN << "SetDevice failed!";
            return -1;
        }

        ret = SetUser(user);
        if (ret != 0) {
            //LOG_WARN << "SetUser failed!";
            return -1;
        }
        ret = SetImp(imp, rtb_request_list);
        if (ret != 0) {
            //LOG_WARN << "SetImp failed!";
            return -1;
        }
    } catch(const std::exception &e) {
        LOG_ERROR << "id:" << Id() << " Wax Request deal failed:" << e.what();
        return -1;
    }
    MON_ADD(ATTR_ADAPTER_CONTROLER_WAX_REQUEST, rtb_request_list.size());
    return 0;
}


int WaxJsonObject::RtbResponse(rtb::BidResponse &rtb_response, EventLoop *loop)
{
    MON_ADD(ATTR_ADAPTER_CONTROLER_WAX_RESPONSE, 1);
    if (rtb_response.no_bid_reason() != 0) {
        return -1;
    }
    response_["id"] = Id();//请求ID
    response_["bidid"] = rtb_response.id(); //DSP给出的该次竞价的ID
    response_["dealid"] = root_["dealid"].asString();
    const OpenDspJson::Value *imp = traceid_impression_map_[rtb_response.trace_id()];
    for (int i = 0; i < rtb_response.bid_seats_size(); ++i) {
        OpenDspJson::Value bids;//bid[]
        rtb::BidSeat* pBidSeat = rtb_response.mutable_bid_seats(i);
        for (int j = 0; j < pBidSeat->bids_size(); ++j) {
            OpenDspJson::Value bid;//针对单次曝光的出价
            rtb::Bid *pBid = pBidSeat->mutable_bids(j);
            SetResponse(rtb_response, pBid, bid, *imp);
            bids["bid"].append(bid);//bid[j]
            ++empty_response_flag_;
        }
        response_["seatbid"].append(bids);
    }
    return 0;
}





///////////////////////////Request adpater///////////////////////////////////
int WaxJsonObject::SetImp(const OpenDspJson::Value &imp, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    for (OpenDspJson::Value::ArrayIndex i = 0; i < imp.size(); ++i) {
        RtbReqSharedPtr request;
        if (imp.size() == 1) {
            request = rtb_request_;
        } else {
            request = Alloc_rtb_req_shared();
            request->CopyFrom(*rtb_request_);
        }
        rtb_request_list.push_back(request);
        const OpenDspJson::Value &impression = imp[i];
        rtb::Impression *pImp = request->add_impressions();
        //sina blog:tagid是广告位，可以用来区分feed和banner。
        //每次请求可以出同一个广告位下的一个或多个广告，
        //每个广告的id就是impid。impid随机生成，每个广告唯一，且10天之内不会重复
        SURE_CVALUE(pImp->set_id, impression["tagid"]);//tagid(广告位ID)
        pImp->set_bidfloor(ASINT(impression["bidfloor"]));//底价,单位是分/千次曝光,即CPM
        auto &rmb = impression["bidfloorcur"];
        if (rmb.asString() == "RMB") {
            pImp->set_bidfloorcur(CurrencyCode[CHINA]);
        }
        pImp->set_secure(NS_HTTP);
        auto &banner = impression["banner"];
        auto &video = impression["video"];
        auto &feed = impression["feed"];
        if (!feed.empty()) {
            //0：不区分类型，1：普通博文，2：card样式博文  品速视频属于feed，但在这里type没对应关系！
            int type = ASINT(feed["type"]);
            switch (type) {
            case 0:
                pImp->add_view_types(rtb::VT_WAX_FEED_NOR);//506
                pImp->add_view_types(rtb::VT_WAX_FEED_ACTIVITY);//507
                pImp->add_view_types(rtb::VT_WAX_FEED_VIDEO);//508
                pImp->add_view_types(rtb::VT_WAX_FEED_GRID);//509
                /*random_shuffle(view_type_vec_.begin(), view_type_vec_.end());
                for (int i = 0; i < view_type_vec_.size(); ++i) {
                    pImp->add_view_types(view_type_vec_[i]);
                }
                */
                break;
            case 1:
                pImp->add_view_types(rtb::VT_WAX_FEED_NOR);
                break;
            case 2:
                pImp->add_view_types(rtb::VT_WAX_FEED_ACTIVITY);
                break;
            default:
                LOG_ERROR << "Unknow feed.type!";
                return -1;//不竞价
            }
        } else if (!banner.empty()) {
            SetImp_Banner(impression, banner, pImp);
            pImp->add_view_types(rtb::VT_WAX_BANNER);//505
        }  else if (!video.empty()) {
            SetImp_Video(impression, video, pImp);
            //pImp->mutable_ext()->set_view_type(rtb:VT_WAX_FEED_VIDEO );
        }

        SetImp_Pmp(impression, pImp);
        rtb::Impression_Ext* pImExt = pImp->mutable_ext();//设置Impression::Ext字段
        SetImp_Ext(impression["ext"], pImExt);
        request->set_trace_id(CreateTraceId());
        traceid_impression_map_[request->trace_id()] = &imp[i];
        LOG_DEBUG << "[<--Rtb Request-->]\n" << request->DebugString();
    }
    return 0;
}


void WaxJsonObject::SetImp_Banner(const OpenDspJson::Value &imp,
                                  const OpenDspJson::Value &banner,
                                  rtb::Impression *pImp)
{
    rtb::Banner* pBan = pImp->mutable_banner();
    SURE_CVALUE(pBan->set_id, imp["id"]);//required field
    if (!pBan->has_id()) {
        pBan->set_id("NULL");
    }
    pBan->set_width(ASINT(banner["w"]));
    pBan->set_height(ASINT(banner["h"]));
}

void WaxJsonObject::SetImp_Video(const OpenDspJson::Value &imp,
                                 const OpenDspJson::Value &video,
                                 rtb::Impression *pImp)
{
    //not imp
}

void WaxJsonObject::SetImp_Pmp(const OpenDspJson::Value &imp, rtb::Impression *pImp)
{
    if (root_["dealid"].empty()) {//required field
        return;
    }
    rtb::PMP *pPmp = pImp->mutable_pmp();
    pPmp->set_private_auction(PA_SPECIFIC_DEALS);
    rtb::PMP_Deal *pDeal= pPmp->add_deals();
    SURE_CVALUE(pDeal->set_id, root_["dealid"]);
    pDeal->set_bidfloorcur(CurrencyCode[CHINA]);
    //deal 价格,单位是分/千次曝光,即CPM
    pDeal->set_bidfloor(ASINT(imp["bidfloor"]));
    // 竞拍类型
    // 1 -- first price auction.
    // 2 -- second price auction.
    // 3 -- the passed bidfloor indicates the apriori agreed upon deal price
    //竞价的方式，目前都是1，即第一竞价法。最高的deal获得竞价成功，取最高出价作为最终胜出价。
    //注：只有当多个Deal同时响应时才互相之间竞价。这个字段和OpenRTB相同。
    pDeal->set_at(ASINT(root_["at"]));
}

void WaxJsonObject::SetImp_Ext(const OpenDspJson::Value &ext, rtb::Impression_Ext* pImExt)
{
    int repeat = 0;
    if(!ext.empty()) {
        repeat = ASINT(ext["repeat"]);
    }
    if (repeat == 0) {
        repeat = 1;
    }
    pImExt->set_ad_num(repeat);
}


int WaxJsonObject::SetApp(const OpenDspJson::Value &app)
{
    if (app.empty()) {
        return -1;
    }
    rtb::App *pApp = rtb_request_->mutable_app();
    SURE_CVALUE(pApp->set_id, app["id"]);
    //app的名称
    SURE_CVALUE(pApp->set_name, app["name"]);
    return 0;
}

int WaxJsonObject::SetDevice(const OpenDspJson::Value &device)
{
    if(device.empty()) {
        return -1;
    }
    rtb::Device* pDevice = rtb_request_->mutable_device();
    SURE_CVALUE(pDevice->set_ip, device["ip"]);
    //user agent
    SURE_CVALUE(pDevice->set_user_agent, device["ua"]);
    //操作系统
    SURE_CVALUE(pDevice->set_os, device["os"]);
    std::string *os = pDevice->mutable_os();
    os->length() > 0 ? boost::to_lower(*os) : void(0);
    //操作系统版本号，如"4.1", "XP"等
    SURE_CVALUE(pDevice->set_os_ver, device["osv"]);
    //当os = ‘ios’时有效, 明文传输，默认为空字符串
    auto &ext_idfa = device["ext"]["idfa"];
    auto &ext_imei = device["ext"]["imei"];
    if (!ext_idfa.empty() && *os == "ios") {
        const char *idfa = ext_idfa.asCString();
        pDevice->set_id(idfa);
        pDevice->set_ifa(idfa);
    } else if (!ext_imei.empty() && *os == "android") {
        //当os = ‘android’时有效
        SURE_CVALUE(pDevice->set_id, ext_imei);
    }
    //型号, 如”iphone_6”
    SURE_CVALUE(pDevice->set_model, device["model"]);
    //链接类型, 0：未知; 1：以太网2：Wifi; 3：移动数据 -未知; 4：2G; 5:3G, 6:4G
    int connectiontype = ASINT(device["connectiontype"]);
    // 0：未知; 1：以太网2：Wifi; 3：移动数据 -未知; 4：2G; 5:3G
    if (connectiontype == 2) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_WIFI);
    } else if (connectiontype == 1) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_ETHERNET);
    } else if (connectiontype == 6) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_4G);
    } else if (connectiontype == 5) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_3G);
    } else if (connectiontype == 3) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA);
    } else if (connectiontype == 4) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_2G);
    } else if (connectiontype == 0) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_UNKNOWN);
    }

    //device->carrier:1 unknown, 2 中国移动, 3 中国联通, 4中国电信
    uint32_t carrier = ASINT(device["carrier"]);
    if (carrier == 46000) {
        pDevice->set_carrier(2);    //中国移动
    } else if (carrier == 46003) {
        pDevice->set_carrier(4);    //中国电信
    } else if (carrier == 46001) {
        pDevice->set_carrier(3);    //中国联通
    } else {
        pDevice->set_carrier(1);    //unknown
    }

    const OpenDspJson::Value &geo = device["geo"];
    if (!geo.empty()) {
        rtb::Geo *rtb_geo = pDevice->mutable_geo();
        !geo["lat"].empty() ? rtb_geo->set_lat(geo["lat"].asFloat()) : void(0);
        !geo["lon"].empty() ? rtb_geo->set_lon(geo["lon"].asFloat()) : void(0);
        !geo["type"].empty() ? rtb_geo->set_type(ASINT(geo["type"])) : void(0);
        !geo["type"].empty() ? rtb_geo->set_type(ASINT(geo["type"])) : void(0);
    }

    return 0;
}

int WaxJsonObject::SetUser(const OpenDspJson::Value &user)
{
    if (user.empty()) {
        return -1;
    }
    rtb::User* pUser = rtb_request_->mutable_user();
    //性别，"M"表示男性，"F"表示女性，为空表示未知
    std::string m = user["gender"].asString();
    if (m == "M") {
        pUser->set_gender(rtb::GENDER_MALE);
    } else if (m == "F") {
        pUser->set_gender(rtb::GENDER_FEMALE);
    } else {
        pUser->set_gender(rtb::GENDER_UNKNOWN);
    }
    //出生年份（Year Of Birth），4位数字，如1988
    int birth = ASINT(user["yob"]);
    if (birth > 0) {
        pUser->set_year_of_birth(birth);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
//DSP若投放微博信息流广告（包括普通博文及品牌大card），每一个竞价广告物料形式为微
//博mid（微博的唯一标识）；待上传素材审核通过后，调用微博生成接口发布博文，获取mid
void WaxJsonObject::SetResponse(const rtb::BidResponse &rtb_resp,
                                const rtb::Bid *pBid,
                                OpenDspJson::Value &bid,
                                const OpenDspJson::Value &imp)
{
    bid["id"] = rtb_resp.trace_id();//pBid->id();//DSP对该次出价分配的ID
    bid["impid"] = imp["id"].asString();//曝光ID
    bid["price"] = pBid->price();

    std::string winnotice;
    BuildWinNotice(rtb_resp, pBid, "s=${AUCTION_PRICE}", winnotice);
    bid["nurl"] = winnotice;//win notice url 竞价成功通知
    //sina blog王方俊: adm，feed返回mid，banner返回图片的地址
    bid["adm"] =  !imp["banner"].empty() ? pBid->image_url() : pBid->ext_cid();
    bid["crid"] = pBid->creative_id();
    OpenDspJson::Value &bid_ext = bid["ext"];
    //wax点击跳转链接需要审核，所以会去取上传的物料信息中的landingpage_url 字段，不支持
    //竞价时对链接做修改
    //bid_ext["ldp"] = pBid->ext().click_url();//目前dest_url与click_url值相同

    //曝光监测URL
    std::string strfeedback;
    BuildExpose(rtb_resp, pBid, "s=__AUCTION_PRICE__&ts=__TS__", strfeedback);
    bid_ext["pm"].append(OpenDspJson::Value(strfeedback));

    //std::string clickUrlEncode;
    std::string urlStr;
    BuildClick(rtb_resp, pBid, "clk=__ACTION_CODE__&ts=__TS__", urlStr);//__ACTION_CODE__目前暂无使用
    bid_ext["cm"].append(OpenDspJson::Value(urlStr));//点击监测地址
}



//////////////////////////////////////////////////////////////////////////////////////

void WaxJsonObject::WaxFailedResponse()
{
    static const string simple_resp = "HTTP/1.1 204 No Content\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      "Content-Type: application/json\r\n\r\n";
    MON_ADD(ATTR_ADAPTER_WAX_EMPTY_RESPONSE, 1);
    //LOG_INFO << "OUT-id_wax_empty_resp:" << Id();
    muduo::net::TcpConnectionPtr ptr = wptr_.lock();
    if (ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(simple_resp.c_str(), simple_resp.length());
    } else {
        MON_ADD(ATTR_ADAPTER_WAX_RESPON_NOT_CONNECTD, 1);
    }
}

void WaxJsonObject::WaxSuccResponse(const std::string &response)
{
    static const string simple_resp = "HTTP/1.1 200 OK\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      "Content-Type: application/json\r\n"
                                      "Content-length:";
    MON_ADD(ATTR_ADAPTER_WAX_SUCC_RESPONSE, 1);
    //LOG_INFO << "OUT-id_wax_resp:" << Id();
    LogStream ss;
    ss << simple_resp << response.length() << "\r\n\r\n";
    ss << response;
    muduo::net::TcpConnectionPtr ptr = wptr_.lock();
    if (ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(ss.buffer().data(), ss.buffer().length());
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_WAX_RESPON_NOT_CONNECTD, 1);
    }
}
