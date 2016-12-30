#include "YoukuJsonObject.h"
#include <vector>

#include "utility/StringCodec.h"
#include <boost/algorithm/string.hpp>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpClient.h>

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;
using namespace OpenDspJson;


extern Configer * volatile configer;

//http://cloud.youku.com/tools#upoad素材上传地址

//每个线程一个配置值，避免初始化时多线程锁的问题
rtb::TrafficSource YoukuJsonObject::source_ = rtb::TS_YOUTU;
std::string YoukuJsonObject::dspid_;
std::string YoukuJsonObject::click_url_;
std::string YoukuJsonObject::expose_url_;
std::string YoukuJsonObject::downloaded_url_;
double YoukuJsonObject::youtu_resp_timeout_;
std::map<std::string, int> YoukuJsonObject::pid_viewtype_map_;
std::map<int, YoukuJsonObject::Youtuadplacements> YoukuJsonObject::pid_object_map_;

YoukuJsonObject::YoukuJsonObject()
{
    //ctor
}

YoukuJsonObject::~YoukuJsonObject()
{
    //dtor
    auto &seatbid = response_["seatbid"];
    if (!seatbid.empty() && seatbid.size() > 0) {
        LOG_INFO << "[<--Youtu Response-->]\n" << OpenDspJson::StyledWriter().write(response_);
        OpenDspJson::FastWriter writer;
        const std::string &json_str = writer.write(response_);
        YouKuSuccResponse(wptr_, json_str);
    } else {
        YouKuFailedResponse(wptr_);
    }
}

void YoukuJsonObject::OnThreadInitStatic()
{

}

void YoukuJsonObject::InitStaticVar(muduo::net::EventLoop *loop)
{
    dspid_ = configer->GetProperty<std::string>("base.youku_dspid", "");//配置的youkudspid
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.youtu_feedback_url", "");
    click_url_ = host + feedback + "/click?";
    expose_url_ = host + feedback + "/feedback?";
    downloaded_url_ = host + feedback + "/download?";

    int timeout = configer->GetProperty<int>("net.youku_response_timeout", 120);
    youtu_resp_timeout_ = (double)timeout / 1000;
    LOG_INFO << "youtu_resp_timeout:" << youtu_resp_timeout_;
    //GetAdPlacements(loop);
    ViewMap();
}

void YoukuJsonObject::ViewMap()
{
    //key-value
    std::vector<std::pair<std::string, std::string>> viewtype_map =
                configer->GetSectionKeys("youtu_view_type_map");

    for(int i = 0; i < viewtype_map.size(); ++i) {
        std::pair<std::string, std::string> &pair = viewtype_map[i];
        MapPid(pair.second, atoi(pair.first.c_str()));
    }
}

void OnHttpResp(const TcpConnectionPtr &ptr, Buffer *buf, Timestamp,
                TcpClient &tcpclient,
                http_parser *parser,
                http_parser_settings *http_setting,
                Buffer &PostData)
{
    size_t n = http_parser_execute(parser, http_setting, buf->peek(), buf->readableBytes());
    LOG_INFO << "YouTu ADPlacement Response:" << buf->peek();
    buf->retrieve(n);
    if(http_body_is_final(parser)) { //TODO:response中content-length值大于实际字节数。造成此函数无效
        tcpclient.disconnect();
        //LOG_INFO << "YouTu ADPlacement Response:" << PostData.peek();
    }
}

int OnHttpData(http_parser *parser, const char *at, size_t length)
{
    Buffer *PostData = (Buffer*)parser->data;
    PostData->append(at, length);
    return 0;
}

void OnConnected(const TcpConnectionPtr &conn, Buffer &PostData)
{
    if(conn->connected()) { //连接成功！
        std::string url = "/dsp/api/adplacements";
        std::string httprequest = "POST /dsp/api/adplacements HTTP/1.1\r\n"
                                  "Host:api.sandbox.yes.youku.com\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Content-length:";
        std::string yoututoken = configer->GetProperty<std::string>("token.youku_token", "");
        std::string dspid = configer->GetProperty<std::string>("base.youku_dspid", "");

        std::string json;
        LogStream ss;
        json.append("{\"dspid\":\"").append(dspid).append("\",");
        json.append("\"token\":\"").append(yoututoken).append("\"}");
        ss << httprequest << json.length() << "\r\n\r\n" << json;
        conn->send(ss.buffer().data(), ss.buffer().length());
    } else { //断开
        if(PostData.readableBytes() > 0) {
            LOG_INFO << "ADPlacement data:" << PostData.peek();
        }
    }
}

void YoukuJsonObject::GetAdPlacements(muduo::net::EventLoop *loop)
{
    std::string hostname = "api.sandbox.yes.youku.com";
    InetAddress address(80);
    if(!InetAddress::resolve(hostname, &address)) {
        return;
    }
    static TcpClient client(loop, address, "youkutmp");
    static http_parser_settings http_setting;
    static http_parser parser;
    static Buffer PostData;
    http_parser_settings_init(&http_setting);
    http_parser_init(&parser, HTTP_RESPONSE);
    parser.data = &PostData;
    http_setting.on_body = OnHttpData;
    client.setConnectionCallback(boost::bind(OnConnected, _1, boost::ref(PostData)));
    client.setMessageCallback(boost::bind(OnHttpResp, _1, _2, _3,
                                          boost::ref(client),
                                          &parser,
                                          &http_setting,
                                          boost::ref(PostData)));
    client.connect();
}



void YoukuJsonObject::MapPid(const std::string &pids, int view_type)
{
    std::vector<std::string> list;
    if(pids.length() == 0) {
        return;
    }
    boost::split(list, pids, boost::is_any_of(";"));
    LOG_INFO << "viewtype:" << view_type << " pidsize:" << list.size();
    auto iter = list.begin();
    for(; iter != list.end(); ++iter) {
        pid_viewtype_map_.insert(std::make_pair(*iter, view_type));//pid - viewtype
    }
}


bool YoukuJsonObject::ParseFromBuff(const HttpRequest*, const char *buff, size_t len)
{
    return reader_.parse(buff, buff + len, root_);
}

//当adxRequest处理失败时，进行的回调
void YoukuJsonObject::OnFailed()
{

}


int YoukuJsonObject::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    //EventLoop *loop = request->GetWConnection().lock()->getLoop();
    //LoopContextPtr loop_ctx = boost::any_cast<LoopContextPtr>(loop->getContext());
    MON_ADD(ATTR_ADAPTER_YOUTU_REQUEST, 1);
    const OpenDspJson::Value &imp = root_["imp"];
    const OpenDspJson::Value &site = root_["site"];
    const OpenDspJson::Value &app = root_["app"];
    const OpenDspJson::Value &device = root_["device"];
    const OpenDspJson::Value &user = root_["user"];
    if(imp.empty() || device.empty() || user.empty()) {
        LOG_ERROR << "Invalid Youku Json object!";
        return -1;
    }

    if(imp.type() != OpenDspJson::arrayValue) {
        LOG_ERROR << "imp must be a array type!";
        return -3;
    }

    if(site.empty() && app.empty()) {
        LOG_ERROR << "site and app both are empty!";
        return -2;
    }

    rtb_request_->set_id(Id());
    int ret;

    try { //jsoncpp在转换值错误时，会抛出异常。因此用try...excption包裹
        if(!site.empty()) { //浏览器发起请求(pc/移动设备)
            ret = SetSite(site);
            if(ret != 0) {
                //LOG_WARN << "SetSite failed!";
                return -1;
            }
        } else { //app发起请求
            ret = SetApp(app);
            if(ret != 0) {
                //LOG_WARN << "SetApp failed!";
                return -1;
            }
        }

        ret = SetDevice(device);
        if(ret != 0) {
            //LOG_WARN << "SetDevice failed!";
            return -1;
        }

        ret = SetUser(user);
        if(ret != 0) {
            //LOG_WARN << "SetUser failed!";
            return -1;
        }
        ret = SetImp(imp);
        if(ret != 0) {
            //LOG_WARN << "SetImp failed!";
            return -1;
        }

    } catch(const std::exception &e) {
        LOG_ERROR << "id:" << Id() << " Youtu Request deal failed:" << e.what();
        return -1;
    }
    LOG_DEBUG << "[-->Youtu Request<--]" << OpenDspJson::StyledWriter().write(root_) <<
              "\n[<--Rtb Request-->]\n" << rtb_request_->DebugString();
    //LOG_INFO << "[<--Rtb Request-->]\n" << rtb_request_->DebugString();
    rtb_request_list.push_back(rtb_request_);
    MON_ADD(ATTR_ADAPTER_CONTROLER_YOUTU_REQUEST, rtb_request_list.size());
    return 0;
}


int YoukuJsonObject::RtbResponse(rtb::BidResponse &rtb_response, EventLoop *loop)
{
    MON_ADD(ATTR_ADAPTER_CONTROLER_YOUTU_RESPONSE, 1);
    if(rtb_response.no_bid_reason() != 0) {
        //LOG_WARN << "no ads of bid from controller, reason code is:" << rtb_response.no_bid_reason();
        return -1;
    }
    response_["id"] = Id();//请求ID
    response_["bidid"] = rtb_response.id(); //DSP给出的该次竞价的ID
    for(int i = 0; i < rtb_response.bid_seats_size(); ++i) {
        OpenDspJson::Value bids;//bid[]
        const rtb::BidSeat &rtb_bidseat = rtb_response.bid_seats(i);
        for(int j = 0; j < rtb_bidseat.bids_size(); ++j) {
            OpenDspJson::Value bid;//针对单次曝光的出价
            const rtb::Bid &rtb_bid = rtb_bidseat.bids(j);
            SetResponse(rtb_response, rtb_bid, bid);
            bids["bid"].append(bid);//bid[j]
        }
        response_["seatbid"].append(bids);
    }
    return 0;
}





///////////////////////////Request adpater///////////////////////////////////
int YoukuJsonObject::SetImp(const OpenDspJson::Value &imp)
{
    for(OpenDspJson::Value::ArrayIndex i = 0; i < imp.size(); ++i) {
        const OpenDspJson::Value &impression = imp[i];
        const std::string tagid = impression["tagid"].asString();
        if(tagid.length() == 0) {
            continue;
        }
        //对于信息流，由于一个位置支持多个模板样式，因此凭tagid获取一个viewtype并
        //不准确
        bool is_native = !impression["native"].empty();
        auto iter = pid_viewtype_map_.find(tagid);
        if (!is_native && iter == pid_viewtype_map_.end()) {
            LOG_WARN << "PID:" << tagid << " is not maped!";
            continue;
        }
        rtb::Impression *pImp = rtb_request_->add_impressions();
        if (!is_native) {
            if (rtb::ViewType_IsValid(iter->second)) {
                pImp->mutable_ext()->set_view_type((rtb::ViewType)iter->second);
                pImp->add_view_types((rtb::ViewType)iter->second);
            } else {
                LOG_ERROR << "view_type:" << iter->second << " Invalid!";
                continue;
            }
        }
        pImp->set_id(tagid);//tagid(广告位ID)。还有一个曝光ID在应答时返回给adx！
        pImp->set_bidfloor(ASINT(impression["bidfloor"]));//底价,单位是分/千次曝光,即CPM
        pImp->set_bidfloorcur(CurrencyCode[CHINA]);
        pImp->set_secure(NS_HTTP);
        if(!impression["banner"].empty()) {
            SetImp_Banner(impression, impression["banner"], pImp);
        } else if(!impression["video"].empty()) {
            SetImp_Video(impression, impression["video"], pImp);
        } else if(is_native) { //信息流
            if (SetImp_Native(impression, impression["native"], pImp) != 0) {
                return -1;
            }
        }
        SetImp_Pmp(impression["pmp"], pImp);
        rtb::Impression_Ext* pImExt = pImp->mutable_ext();//设置Impression::Ext字段
        SetImp_Ext(impression["ext"], pImExt);
        rtb_request_->set_trace_id(CreateTraceId());//创建一个trace_id
        traceid_impression_[rtb_request_->trace_id()] = &imp[i];//保存有效的广告位映射，应答时使用
        return 0;//多广告位的话只取一个
    }
    return -1;
}


void YoukuJsonObject::SetImp_Banner(const OpenDspJson::Value &imp,
                                    const OpenDspJson::Value &banner,
                                    rtb::Impression *pImp)
{
    rtb::Banner* pBan = pImp->mutable_banner();
    pBan->set_id(pImp->id());
    pBan->set_width(ASINT(banner["w"]));
    pBan->set_height(ASINT(banner["h"]));
    pBan->set_position(ASINT(banner["pos"]));
    //Youku流量中，未指定banner允许的创意mime类型
}

void YoukuJsonObject::SetImp_Video(const OpenDspJson::Value &imp,
                                   const OpenDspJson::Value &video,
                                   rtb::Impression *pImp)
{
    rtb::Video *pRtbVideo = pImp->mutable_video();
    //MIME编码， 目前支持： video/x-flv，application/x-shockwave-flash。
    //text/html:baner的动态素材
    const OpenDspJson::Value &mimes = video["mimes"];
    for(OpenDspJson::Value::ArrayIndex i = 0; i < mimes.size(); ++i) {
        //支持播放的视频格式，目前支持： video/x-flv，application/x-shockwave-flash。
        SURE_CVALUE(pRtbVideo->add_formats, mimes[i]);
    }


    // "In-stream/linear" 指的是：为了看视频内容，强制用户必须观看的前贴/中贴/后贴广告
    // “Overlay/non-linear” 指的是：在视频内容上展示的广告。
    //广告展现样式 set VideoLinearity
    //0: "未知";
    //1："instream/linear"即线性贴片素材，包括前贴中贴后贴;
    //2:"overlay/nonlinear"即视频播放中的悬浮广告;
    //3："pause"即视频播放暂停中的悬浮广告;
    //4:"fullscreen"即视频全屏播放时的悬浮广告;
    int line = ASINT(video["linearity"]);
    if(rtb::VideoLinearity_IsValid(line)) {
        pRtbVideo->set_linearity((rtb::VideoLinearity)line);
    } else {
        LOG_WARN << "pid:" << imp["tagid"].asString() << " has new linearity:" << line;
    }

    // set min_duration 视频广告最短播放时长，单位是秒
    if (!video["minduration"].empty()) {
        pRtbVideo->set_min_duration(ASINT(video["minduration"]));
    }

    // set max_duration
    if (!video["maxduration"].empty()) {
        pRtbVideo->set_max_duration(ASINT(video["maxduration"]));
    }

    // set video protocol
    //pRtbVideo->set_protocol(rtb::VIDEO_PROTOCOL_VAST_30);

    // set width of adz or player
    // set heigh of adz or player
    if (!video["w"].empty()) {
        pRtbVideo->set_width(ASINT(video["w"]));
    }
    if(!video["h"].empty()) {
        pRtbVideo->set_height(ASINT(video["h"]));
    }
    // set start delay 该字段仅在linearity=1时有效；线性贴片，0：前帖；-1：中贴；-2：后贴。
    // 单位(秒)
    // 如果值大于0，则是中贴，该值表示视频播放多少秒以后出广告
    // 0 -- 前贴
    // 1 - 中贴
    // 2 - 后贴
    if (!video["startdelay"].empty()) {
        int sd = ASINT(video["startdelay"]);
        if(sd == 0) {
            pRtbVideo->set_start_delay((poseidon::rtb::VideoStartDelay)0);
        } else if(sd == -1) {
            pRtbVideo->set_start_delay((poseidon::rtb::VideoStartDelay)1);
        } else if(sd == -2) {
            pRtbVideo->set_start_delay((poseidon::rtb::VideoStartDelay)2);
        }
    }


    // set content
    //把site或app里的content内容放在rtb::impression::video::content中，不再分别放在
    //rtb::site或rtb::app中了。也就是说，多个不同的rtb::impression::video有相同的content内容
    rtb::Content* pRtbVideoContent = pRtbVideo->mutable_ext()->mutable_content();
    const OpenDspJson::Value &content =
        ((!root_["site"].empty()) ? root_["site"]["content"] : root_["app"]["content"]);
    //site或app只有一个content
    SetSiteApp_Content(content, pRtbVideoContent);
}

int YoukuJsonObject::SetImp_Native(const OpenDspJson::Value &imp,
                                    const OpenDspJson::Value &native,
                                    rtb::Impression *pImp)
{
    auto &native_template_ids = native["native_template_ids"];
    //支持的模板id集合，一个（广告）位置支持多个模板样式
    for(OpenDspJson::Value::ArrayIndex i = 0; i < native_template_ids.size(); ++i) {
        const char *id = native_template_ids[i].asCString();
        if (id && memcmp(id, "100", 3) == 0) {
            pImp->add_view_types(rtb::VT_YT_MOBILE_APP_NATIVE_100);
        } else {
            return -1;
        }
    }
    //信息流规范，跟优土那边沟通过了，先不理会，以审核为准
    /*
    for(OpenDspJson::Value::ArrayIndex i = 0; i < assets.size(); ++i) {
        auto &asset = assets[i];
        std::string &native_template_id = asset["native_template_id"].asString();
        int len = ASINT(asset["title"]["len"]);//广告标题长度限制
        int w = ASINT(asset["image_url"]["w"]);
        int h = ASINT(asset["image_url"]["h"]);
    }
    */
    return 0;
}

void YoukuJsonObject::SetImp_Pmp(const OpenDspJson::Value &pmp, rtb::Impression *pImp)
{
    if(pmp.empty()) {
        return;
    }
    rtb::PMP *pPmp = pImp->mutable_pmp();
    pPmp->set_private_auction(PA_SPECIFIC_DEALS);

    const OpenDspJson::Value &deals = pmp["deals"];
    for(OpenDspJson::Value::ArrayIndex i = 0; i < deals.size(); ++i) {
        const OpenDspJson::Value &deal = deals[i];
        rtb::PMP_Deal *pDeal= pPmp->add_deals();
        SURE_CVALUE(pDeal->set_id, deal["id"]);
        pDeal->set_bidfloorcur(CurrencyCode[CHINA]);
        //deal 价格,单位是分/千次曝光,即CPM
        pDeal->set_bidfloor(ASINT(deal["bidfloor"]));
        // 竞拍类型
        // 1 -- first price auction.
        // 2 -- second price auction.
        // 3 -- the passed bidfloor indicates the apriori agreed upon deal price
        //竞价的方式，目前都是1，即第一竞价法。最高的deal获得竞价成功，取最高出价作为最终胜出价。
        //注：只有当多个Deal同时响应时才互相之间竞价。这个字段和OpenRTB相同。
        pDeal->set_at(ASINT(deal["at"]));
    }
}

void YoukuJsonObject::SetImp_Ext(const OpenDspJson::Value &ext, rtb::Impression_Ext* pImExt)
{
    int repeat = 1;
    if(!ext.empty()) {
        repeat = ASINT(ext["repeat"]);
    }
    pImExt->set_ad_num(repeat);
}


int YoukuJsonObject::SetSite(const OpenDspJson::Value &site)
{
    rtb::Site *pSite = rtb_request_->mutable_site();

    //pSite->set_site_id(vPid[2]);
    //媒体网站名称
    SURE_CVALUE(pSite->set_name, site["name"]);
    //当前页面URL
    SURE_CVALUE(pSite->set_page, site["page"]);
    //Referrer URL
    SURE_CVALUE(pSite->set_ref, site["ref"]);
    /*
    if (!publisherId.empty()) { //TODO
        rtb::Publisher* pPub = pSite->mutable_publisher();
        pPub->set_id(publisherId);
    }
    */
    //set content
    //const OpenDspJson::Value &content = site["content"];
    //rtb::Content *pContent = pSite->mutable_content();
    //SetSiteApp_Content(content, pContent);///改为在video中设值
    return 0;
}

//不管是site还是app均有content结构
void YoukuJsonObject::SetSiteApp_Content(const OpenDspJson::Value &content, rtb::Content *pContent)
{
    if(content.empty()) {
        return;
    }
    char *title = (char*)content["title"].asCString();
    if(title == NULL) {
        return;
    }
    //视频标题名称
    pContent->set_title(title);
    std::vector<std::string> vec;
    const std::string &keywords = content["keywords"].asString();
    //视频标签关键字，如果是多个关键字，则使用英文竖线分隔
    boost::split(vec, keywords, boost::is_any_of("|"));
    for(size_t i = 0; i < vec.size(); ++i) {
        if(vec[i].length() == 0) {
            continue;
        }
        std::string &v = vec[i];
        pContent->add_keywords(v);
    }
    const OpenDspJson::Value &ext = content["ext"];
    if(!ext.empty()) {
        rtb::Content::Ext *pExt = pContent->mutable_ext();
        //视频的频道ID，例如"a"。具体的频道列表，参见字典文件Youku ADX频道列表
        if(!ext["channel"].empty()) {
            rtb::Content::Ext::Direct *direct = pExt->add_direct();
            direct->set_key("channel");
            direct->set_value(ext["channel"].asString());
        }

        if(!ext["cs"].empty()) {
            //二级频道ID。具体的二级频道列表信息，参见字典文件Youku ADX二级频道列表
            rtb::Content::Ext::Direct *direct1 = pExt->add_direct();
            direct1->set_key("cs");
            direct1->set_value(ext["cs"].asString());
        }

        if(!ext["usr"].empty()) {
            //视频上传者id，优酷里上传视频的用户ID
            rtb::Content::Ext::Direct *direct2 = pExt->add_direct();
            direct2->set_key("usr");
            direct2->set_value(ext["usr"].asString());
        }

        if(!ext["s"].empty()) {
            //节目id
            rtb::Content::Ext::Direct *direct3 = pExt->add_direct();
            direct3->set_key("s");
            direct3->set_value(ext["s"].asString());
        }

        if(!ext["vid"].empty()) {
            //视频id
            rtb::Content::Ext::Direct *direct4 = pExt->add_direct();
            direct4->set_key("vid");
            direct4->set_value(ext["vid"].asString());
        }
    }
}

int YoukuJsonObject::SetApp(const OpenDspJson::Value &app)
{
    if(app.empty()) {
        return -1;
    }
    rtb::App *pApp = rtb_request_->mutable_app();
    //pp的名称，一般是"优酷客户端"，或者"土豆客户端"。
    SURE_CVALUE(pApp->set_name, app["name"]);

    //视频的内容相关信息。只有视频贴片类型的广告位才会有这个字段，内容与site.content对象相同
    //const OpenDspJson::Value &content = app["content"];
    //rtb::Content *pContent = pApp->mutable_content();
    //SetSiteApp_Content(content, pContent);
    return 0;
}

int YoukuJsonObject::SetDevice(const OpenDspJson::Value &device)
{
    if(device.empty()) {
        return -1;
    }
    rtb::Device* pDevice = rtb_request_->mutable_device();
    SURE_CVALUE(pDevice->set_ip, device["ip"]);
    //user agent
    SURE_CVALUE(pDevice->set_user_agent, device["ua"]);

    //使用MD5哈希的Device ID，在优酷的移动流量里，这个字段是IMEI的md5值
    //pDevice->set_id(device["didmd5"].asString());//device_id
    //操作系统
    //boost::to_lower(os);
    //std::transform(os.begin(), os.end(), os.begin(), tolower);
    SURE_CVALUE(pDevice->set_os, device["os"]);
    std::string *os = pDevice->mutable_os();
    os->length() > 0 ? boost::to_lower(*os) : void(0);
    //操作系统版本号，如"4.1", "XP"等
    SURE_CVALUE(pDevice->set_os_ver, device["osv"]);
    //设备类型，和0—手机，1—平板，2—PC，3—互联网电视。
    int devicetype = ASINT(device["devicetype"]);
    if(devicetype == 0) { //手机
        pDevice->set_device_type(DT_PHONE);
    } else if(devicetype == 1) { //平板
        pDevice->set_device_type(DT_TABLET);
    } else if(devicetype == 2) { //PC
        pDevice->set_device_type(DT_PC);
    } else if(devicetype == 3) { //互联网电视
        pDevice->set_device_type(DT_TV);
    }
    //当os = ‘ios’时有效, 明文传输，默认为空字符串
    if(!device["idfa"].empty() && *os == "ios") {
        const char *idfa = device["idfa"].asCString();
        pDevice->set_id(idfa);
        pDevice->set_ifa(idfa);
    } else {
        //当os = ‘android’时有效，明文传输，默认为空字符串
        //if(!device["androidid"].empty() && device["os"].asString() == "android")
        //IMEI, 明文传输，默认为空字符串
        SURE_CVALUE(pDevice->set_id, device["imei"]);
    }
    //MAC地址，明文传输，默认为空字符串
    if(!device["mac"].empty() && pDevice->id().length() == 0) {
        pDevice->set_id(device["mac"].asCString());
    }

    //制造厂商,如“apple”“Samsung”“Huawei“，默认为空字符串
    SURE_CVALUE(pDevice->set_make, device["make"]);

    //型号, 如”iphoneA1530”，默认为空字符串
    SURE_CVALUE(pDevice->set_model, device["model"]);
    //链接类型, 0：未知; 1：以太网2：Wifi; 3：移动数据 -未知; 4：2G; 5:3G
    int connectiontype = ASINT(device["connectiontype"]);
    // 0：未知; 1：以太网2：Wifi; 3：移动数据 -未知; 4：2G; 5:3G
    if(connectiontype == 2) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_WIFI);
    } else if(connectiontype == 1) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_ETHERNET);
    } else if(connectiontype == 6) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_4G);
    } else if(connectiontype == 5) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_3G);
    } else if(connectiontype == 3) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA);
    } else if(connectiontype == 4) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_2G);
    } else if(connectiontype == 0) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_UNKNOWN);
    }


    //device["carrier"] :0：wifi；1：中国移动；2：中国联通：3：中国电信；4：其他；5：未识别
    //device->carrier:1 unknown, 2 中国移动, 3 中国联通, 4中国电信
    int carrier = ASINT(device["carrier"]);
    if(carrier == 1) {
        pDevice->set_carrier(2);    //中国移动
    } else if(carrier == 3) {
        pDevice->set_carrier(4);    //中国电信
    } else if(carrier == 2) {
        pDevice->set_carrier(3);    //中国联通
    } else if(carrier == 0) {
        pDevice->set_carrier(1);    //unknown
    } else {
        pDevice->set_carrier(1);    //unknown
    }

    int w = ASINT(device["screenwidth"]);
    int h = ASINT(device["screenhight"]);
    if(w > 0 && h > 0) {
        char size[32];
        snprintf(size, sizeof(size), "%dx%d", w, h);
        pDevice->mutable_ext()->set_dev_resolution(size);
    }
    return 0;
}

int YoukuJsonObject::SetUser(const OpenDspJson::Value &user)
{
    if(user.empty()) {
        return -1;
    }
    rtb::User* pUser = rtb_request_->mutable_user();
    //用户ID。在PC流量中，这个ID是youku.com的cookie中的用户ID字段；
    //在移动流量中，该字段具体取值方式，参见附录UserID取值规则
    SURE_CVALUE(pUser->set_id, user["id"]);

    //性别，"M"表示男性，"F"表示女性，为空表示未知
    std::string m = user["gender"].asString();
    if(m == "M") {
        pUser->set_gender(rtb::GENDER_MALE);
    } else if(m == "F") {
        pUser->set_gender(rtb::GENDER_FEMALE);
    } else {
        pUser->set_gender(rtb::GENDER_UNKNOWN);
    }
    //出生年份（Year Of Birth），4位数字，如1988
    int birth = ASINT(user["yob"]);
    if(birth > 0) {
        pUser->set_year_of_birth(birth);
    }
    //用户相关的标签信息（只用于百度）描述见user.tag
    const OpenDspJson::Value &tag = user["tag"];
    if(!tag.empty()) {
        //not imp
        /*
        ASINT(tag["1"]);//品牌用户的可能性
        ASINT(tag["2"]);//游戏用户的可能性
        ASINT(tag["3"]);//电商用户的可能性
        ASINT(tag["4"]);//中小企业用户的可能性
        ASINT(tag["5"]);//道长用户的可能性
        */
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////
void YoukuJsonObject::SetResponse(const rtb::BidResponse &rtb_resp,
                                  const rtb::Bid &rtb_bid,
                                  OpenDspJson::Value &bid)
{
    const OpenDspJson::Value *imp = traceid_impression_[rtb_resp.trace_id()];
    if(!imp || (imp && imp->empty())) {
        return;    //impossible
    }
    bool is_native = !(*imp)["native"].empty();
    if (is_native) {
        if (rtb_bid.view_type() == rtb::VT_YT_MOBILE_APP_NATIVE_100) {
            bid["native"]["native_template_id"] = "100";
        }
    }
    bid["id"] = rtb_bid.id();//DSP对该次出价分配的ID
    bid["impid"] = (*imp)["id"];//返回曝光id
    bid["price"] = (double)rtb_bid.price();

    std::string strfeedback;
    BuildExpose(rtb_resp, &rtb_bid, "s=${AUCTION_PRICE}", strfeedback);
    bid["nurl"] = strfeedback;//win notice url 竞价成功通知

    //广告素材URL。如果是动态创意，这个字段存放的是创意的HTML标签，标签中支持两种宏替换，
    //%%CLICK_URL_ESC%%（Exchange的点击监测地址）和%%WINNING_PRICE%%（竞价最终价格）

    //RTB返回时，adm保存视频url(flv)，image_url保存banner的url
    //banner广告位支持gif、jpg、png、swf、flv、x和c七种素材格式
    //video广告位只支持swf、flv和x三种素材格式
    bid["adm"] = (!(*imp)["banner"].empty()) ? rtb_bid.image_url() : rtb_bid.adm();

    if(bid["adm"].asString().length() == 0 && (*imp)["native"].empty()) {
        LOG_WARN << "bid[\"adm\"] is null!";
        bid["adm"] = rtb_bid.image_url().length() > 0 ? rtb_bid.image_url() : rtb_bid.adm();
    }

    bid["crid"] = rtb_bid.creative_id();
    //DSP参加的deal id
    if(!rtb_bid.deal_id().empty()) {
        bid["dealid"] = rtb_bid.deal_id();
    }

    OpenDspJson::Value &bid_ext = bid["ext"];
    //点击目标URL
    bid_ext["ldp"] = rtb_bid.ext().click_url();//目前dest_url与click_url值相同
    //曝光监测URL
    //OpenDspJson::Value feedback(strfeedback);
    //bid_ext["pm"].append(feedback);


    //std::string clickUrlEncode;
    std::string urlStr;
    //unsigned int crc;
    //std::string encodestr;
    //StringEncode(rtb_bid.ext().click_url(), crc, encodestr);
    //StringCodec::UrlEncode(encodestr, clickUrlEncode);
    //点击跳转地址不再传递，由ldp字段执行即可
    BuildClick(rtb_resp, &rtb_bid, "", urlStr);
    OpenDspJson::Value clickUrl(urlStr);
    bid_ext["cm"].append(clickUrl);//点击监测地址
}



//////////////////////////////////////////////////////////////////////////////////////

void YoukuJsonObject::YouKuFailedResponse(TcpConnectionWptr &wptr)
{
    static const string simple_resp = "HTTP/1.1 204 No Content\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      "Content-Type: application/json\r\n\r\n";
    MON_ADD(ATTR_ADAPTER_YOUTU_EMPTY_RESPONSE, 1);
    //LOG_INFO << "OUT-id_youtu_empty_resp:" << Id();
    muduo::net::TcpConnectionPtr ptr = wptr.lock();
    if(ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(simple_resp.c_str(), simple_resp.length());
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_YOUTU_RESPON_NOT_CONNECTD, 1);
    }
}

void YoukuJsonObject::YouKuSuccResponse(TcpConnectionWptr &wptr, const std::string &response)
{
    static const string simple_resp = "HTTP/1.1 200 OK\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      "Content-Type: application/json\r\n"
                                      "Content-length:";
    MON_ADD(ATTR_ADAPTER_YOUTU_SUCC_RESPONSE, 1);
    //LOG_INFO << "OUT-id_youtu_resp:" << Id();
    LogStream ss;
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
        MON_ADD(ATTR_ADAPTER_YOUTU_RESPON_NOT_CONNECTD, 1);
    }
}
