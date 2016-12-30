#include "ChanceJsonObject.h"
#include <vector>

#include "utility/StringCodec.h"
#include <boost/algorithm/string.hpp>

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;
using namespace OpenDspJson;


extern Configer * volatile configer;

rtb::TrafficSource ChanceJsonObject::source_ = rtb::TS_CHANCE;
std::string ChanceJsonObject::click_url_;
std::string ChanceJsonObject::expose_url_;
std::string ChanceJsonObject::downloaded_url_;
double ChanceJsonObject::chance_resp_timeout_;

ChanceJsonObject::ChanceJsonObject() : JsonObject()
{
    //ctor
}

ChanceJsonObject::~ChanceJsonObject()
{
    //dtor
}

void ChanceJsonObject::OnThreadInitStatic()
{

}

void ChanceJsonObject::InitStaticVar()
{
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.chance_feedback_url", "");
    click_url_ = host + feedback + "/click?";
    expose_url_ = host + feedback + "/feedback?";
    downloaded_url_ = host + feedback + "/download?";

    int timeout = configer->GetProperty<int>("net.chance_response_timeout", 200);
    chance_resp_timeout_ = (double)timeout / 1000;
    LOG_INFO << "chance_resp_timeout:" << chance_resp_timeout_;
    //LOG_INFO << "click_url:" << click_url_ << " expose_url:" << expose_url_ <<
    //         " downloaded_url" << downloaded_url_ << " timeout:" << chance_resp_timeout_;
}


bool ChanceJsonObject::ParseFromBuff(const HttpRequest*, const char *buff, size_t len)
{
    return reader_.parse(buff, buff + len, root_);
}

//当adxRequest处理失败时，进行的回调
void ChanceJsonObject::OnFailed()
{
    ChanceFailedResponse(wptr_);
}


int ChanceJsonObject::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    MON_ADD(ATTR_ADAPTER_CHANCE_REQUEST, 1);

    dspid_ = root_["pid"].asString();//11880815
    OpenDspJson::Value &imp = root_["adinfo"];
    OpenDspJson::Value &app = root_["appinfo"];
    OpenDspJson::Value &device = root_["device"];
    if(imp.empty() || device.empty()) {
        LOG_ERROR << "Invalid chance Json object!";
        return -1;
    }
    rtb_request_->set_id(Id());
    try {
        int ret = SetImp(imp);
        if(ret != 0) {
            LOG_ERROR << "SetImp failed!";
            return -1;
        }

        ret = SetApp(app);
        if(ret != 0) {
            LOG_ERROR << "SetApp failed!";
            return -1;
        }

        ret = SetDevice(device);
        if(ret != 0) {
            LOG_ERROR << "SetDevice failed!";
            return -1;
        }
    } catch(const std::exception &e) {
        LOG_ERROR << "sid:" << Id() << " chance Request deal failed:" << e.what();
        return -1;
    }
    rtb_request_->set_trace_id(CreateTraceId());
    rtb_request_list.push_back(rtb_request_);
    LOG_DEBUG << "[-->Chance Request<--]\n" << OpenDspJson::StyledWriter().write(root_) << "\n" <<
              "[<--Rtb Request-->]\n" << rtb_request_->DebugString();
    MON_ADD(ATTR_ADAPTER_CONTROLER_CHANCE_REQUEST, rtb_request_list.size());

    return 0;
}


int ChanceJsonObject::RtbResponse(rtb::BidResponse &rtb_response, EventLoop *loop)
{
    MON_ADD(ATTR_ADAPTER_CONTROLER_CHANCE_RESPONSE, 1);

    if(rtb_response.no_bid_reason() != 0) {
        //LOG_WARN << "no ads of bid from controller, reason code is:" << rtb_response.no_bid_reason();
        return -1;
    }

    response_["sid"] = Id();//请求ID
    response_["version"] = root_["version"].asString();

    response_["err"] = "0";
    response_["msg"] = "";
    int adnum = 0;
    for (int i = 0; i < rtb_response.bid_seats_size(); ++i) {
        rtb::BidSeat* pBidSeat = rtb_response.mutable_bid_seats(i);
        for (int j = 0; j < pBidSeat->bids_size(); ++j) {
            OpenDspJson::Value bid;//针对单次曝光的出价
            rtb::Bid *pBid = pBidSeat->mutable_bids(j);
            SetResponse(rtb_response, pBid, bid);
            response_["ads"].append(bid);
            ++adnum;
        }

    }

    response_["adnum"] = adnum;

    //写两次，可改为一次写出
    LOG_INFO << "[<--Chance Response-->]\n" << OpenDspJson::StyledWriter().write(response_);
    OpenDspJson::FastWriter writer;
    const std::string &json_str = writer.write(response_);

    MON_ADD(ATTR_ADAPTER_CHANCE_SUCC_RESPONSE, 1);
    //LOG_INFO << "OUT-id_chance_resp:" << Id();
    ChanceSuccResponse(wptr_, json_str);
    return 0;
}





///////////////////////////Request adpater///////////////////////////////////
int ChanceJsonObject::SetImp(const OpenDspJson::Value &imp)//adinfo
{
    rtb::Impression *pImp = rtb_request_->add_impressions();
    SURE_CVALUE(pImp->set_id, imp["id"]);//广告位ID
    pImp->set_bidfloor(ASINT(imp["bidfloor"]) * 100);//底价,单位是元。目前此字段暂无使用
    pImp->set_bidfloorcur(CurrencyCode[CHINA]);
    pImp->set_secure(NS_HTTP);

    rtb::Banner* pBan = pImp->mutable_banner();
    SURE_CVALUE(pBan->set_id, imp["id"]);
    //注意，chance的imp还包含广告位长宽：w, h
    //pBan->set_width(ASINT(imp["imgw"]));
    //pBan->set_height(ASINT(imp["imgh"]));

    pBan->set_width(ASINT(imp["w"]));
    pBan->set_height(ASINT(imp["h"]));

    if(ASINT(imp["ctype"]) == 1) { //目前只支持1，图片广告
        pBan->add_formats("img/jepg");
        pBan->add_formats("img/png");
    }

    //pBan->set_position(ASINT(imp["pos"]));

    if(ASINT(imp["adtype"]) == 2) { //插屏
        pImp->mutable_ext()->set_view_type(rtb::VT_WL_POP_WINDOW);
    } else if(ASINT(imp["adtype"]) == 1) {
        pImp->mutable_ext()->set_view_type(rtb::VT_WL_BANNER);
    } else if(ASINT(imp["adtype"]) == 21) {
        pImp->mutable_ext()->set_view_type(rtb::VT_WL_FEEDS);
    }

    rtb::Impression_Ext* pImExt = pImp->mutable_ext();//设置Impression::Ext字段
    pImExt->set_ad_num(ASINT(imp["adnum"]));
    return 0;
}


int ChanceJsonObject::SetApp(const OpenDspJson::Value &app)
{
    if(app.empty()) {
        return -1;
    }
    rtb::App *pApp = rtb_request_->mutable_app();
    //app的名称。
    SURE_CVALUE(pApp->set_name, app["name"]);
    SURE_CVALUE(pApp->set_id, app["id"]);
    SURE_CVALUE(pApp->set_bundle, app["bundle"]);
    return 0;
}

int ChanceJsonObject::SetDevice(const OpenDspJson::Value &device)
{
    if(device.empty()) {
        return -1;
    }
    rtb::Device* pDevice = rtb_request_->mutable_device();
    SURE_CVALUE(pDevice->set_ip, device["ip"]);
    //user agent
    SURE_CVALUE(pDevice->set_user_agent, device["ua"]);
    //操作系统
    int i_os = ASINT(device["os"]);
    std::string os;
    if(i_os == 0) {
        os = "ios";
    } else if(i_os == 1) {
        os = "android";
    }
    pDevice->set_os(os);
    //操作系统版本号，如"4.1", "XP"等
    SURE_CVALUE(pDevice->set_os_ver, device["osv"]);
    //设备类型，和0—手机，1—平板，2—PC，3—互联网电视。
    pDevice->set_device_type(DT_PHONE);

    //当os = ‘ios’时有效, 明文传输，默认为空字符串
    if(!device["idfa"].empty() && os == "ios") {
        const char *idfa = device["idfa"].asCString();
        pDevice->set_id(idfa);
        pDevice->set_ifa(idfa);
    }
    //IMEI
    SURE_CVALUE(pDevice->set_id, device["imei"]);

    //MAC地址
    if(!device["mac"].empty() && pDevice->id().empty()) {
        pDevice->set_id(device["mac"].asCString());
    }
    //android id
    if(!device["anid"].empty() && pDevice->id().empty()) {
        pDevice->set_id(device["anid"].asCString());
    }
    //imsi
    if(!device["imsi"].empty() && pDevice->id().empty()) {
        pDevice->set_id(device["imsi"].asCString());
    }


    //制造厂商,如“apple”“Samsung”“Huawei“，默认为空字符串
    SURE_CVALUE(pDevice->set_make, device["brand"]);

    //型号, 如”iphoneA1530”，默认为空字符串
    SURE_CVALUE(pDevice->set_model, device["model"]);

    int connectiontype = ASINT(device["net"]);
    // 0：未知; 1：Wifi; 2：2G; 3:3G, 4:代理 5:其他 6:4G
    switch(connectiontype) {
    case 1:
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_WIFI);
        break;
    case 2:
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_2G);
        break;
    case 3:
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_3G);
        break;
    case 6:
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_4G);
        break;
    default:
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_UNKNOWN);
        break;
    }

    if(!device["res_w"].empty() && !device["res_h"].empty()) {
        char size[32];
        snprintf(size, sizeof(size), "%dx%d", ASINT(device["res_w"]),
                 ASINT(device["res_h"]));
        pDevice->mutable_ext()->set_dev_resolution(size);

    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
void ChanceJsonObject::SetResponse(const rtb::BidResponse &rtb_resp, const rtb::Bid *pBid, OpenDspJson::Value &bid)
{
    bid["adid"] = pBid->ext().adgroup_id();
    bid["adtype"] = ASINT(root_["adinfo"]["adtype"]);
    //bid["title"] = ""; //ctype不为1时有效
    //bid["content"] = ""; //ctype不为1时有效
    OpenDspJson::Value img_url(pBid->image_url());
    bid["imgurls"].append(img_url);
    bid["imgnum"] = 1;
    bid["ctype"] = ASINT(root_["adinfo"]["ctype"]);
    bid["link"] = pBid->ext().click_url();//落地页
    bid["w"] = pBid->w();
    bid["h"] = pBid->h();
    if(pBid->ext().download_url().length() > 0) {
        bid["targettype"] = 1;//不严谨？判断open_type？
    } else {
        bid["targettype"] = 2;
    }

    std::string clickUrlEncode;
    std::string urlStr;
    //unsigned int crc;
    //std::string encodestr;
    //StringEncode(pBid->ext().click_url(), crc, encodestr);
    //StringCodec::UrlEncode(encodestr, clickUrlEncode);
    clickUrlEncode = "u=" + clickUrlEncode;
    BuildClick(rtb_resp, pBid, clickUrlEncode, urlStr);
    OpenDspJson::Value clickUrl(urlStr);
    bid["click"].append(clickUrl);//点击监测地址

    std::string strfeedback;
    BuildExpose(rtb_resp, pBid, "", strfeedback);
    OpenDspJson::Value expose_url(strfeedback);
    bid["view"].append(expose_url);//曝光监测地址

    //bid["price"] = (double)pBid->price();
    //bid["pricetype"] = (double)pBid->price();

}


//////////////////////////////////////////////////////////////////////////////////////

void ChanceJsonObject::ChanceFailedResponse(TcpConnectionWptr &wptr)
{
    MON_ADD(ATTR_ADAPTER_CHANCE_EMPTY_RESPONSE, 1);
    //LOG_INFO << "OUT-id_chance_empty_resp:" << Id();

    response_["version"] = root_["version"].asString();
    response_["sid"] = Id();
    response_["err"] = "-1";
    response_["msg"] = "empty resp";
    OpenDspJson::FastWriter writer;
    const std::string &json_str = writer.write(response_);
    ChanceSuccResponse(wptr, json_str);
}

void ChanceJsonObject::ChanceSuccResponse(TcpConnectionWptr &wptr, const std::string &response)
{
    static const string simple_resp = "HTTP/1.1 200 OK\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      "Content-Type: application/json\r\n"
                                      "Content-length:";

    //注意:LogStream里的buff大小只有smallsize,15KB。大了会丢数据！
    /*std::stringstream*/LogStream ss;
    ss << simple_resp << response.length() << "\r\n\r\n";
    ss << response;
    muduo::net::TcpConnectionPtr ptr = wptr.lock();
    if(ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        //const std::string &buf = ss.str();
        //ptr->send(buf.c_str(), buf.length());
        ptr->send(ss.buffer().data(), ss.buffer().length());
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_CHANCE_RESPON_NOT_CONNECTD, 1);
    }
}
