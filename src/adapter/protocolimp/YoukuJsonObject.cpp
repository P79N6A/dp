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

//http://cloud.youku.com/tools#upoad�ز��ϴ���ַ

//ÿ���߳�һ������ֵ�������ʼ��ʱ���߳���������
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
    dspid_ = configer->GetProperty<std::string>("base.youku_dspid", "");//���õ�youkudspid
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
    if(http_body_is_final(parser)) { //TODO:response��content-lengthֵ����ʵ���ֽ�������ɴ˺�����Ч
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
    if(conn->connected()) { //���ӳɹ���
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
    } else { //�Ͽ�
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

//��adxRequest����ʧ��ʱ�����еĻص�
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

    try { //jsoncpp��ת��ֵ����ʱ�����׳��쳣�������try...excption����
        if(!site.empty()) { //�������������(pc/�ƶ��豸)
            ret = SetSite(site);
            if(ret != 0) {
                //LOG_WARN << "SetSite failed!";
                return -1;
            }
        } else { //app��������
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
    response_["id"] = Id();//����ID
    response_["bidid"] = rtb_response.id(); //DSP�����ĸôξ��۵�ID
    for(int i = 0; i < rtb_response.bid_seats_size(); ++i) {
        OpenDspJson::Value bids;//bid[]
        const rtb::BidSeat &rtb_bidseat = rtb_response.bid_seats(i);
        for(int j = 0; j < rtb_bidseat.bids_size(); ++j) {
            OpenDspJson::Value bid;//��Ե����ع�ĳ���
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
        //������Ϣ��������һ��λ��֧�ֶ��ģ����ʽ�����ƾtagid��ȡһ��viewtype��
        //��׼ȷ
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
        pImp->set_id(tagid);//tagid(���λID)������һ���ع�ID��Ӧ��ʱ���ظ�adx��
        pImp->set_bidfloor(ASINT(impression["bidfloor"]));//�׼�,��λ�Ƿ�/ǧ���ع�,��CPM
        pImp->set_bidfloorcur(CurrencyCode[CHINA]);
        pImp->set_secure(NS_HTTP);
        if(!impression["banner"].empty()) {
            SetImp_Banner(impression, impression["banner"], pImp);
        } else if(!impression["video"].empty()) {
            SetImp_Video(impression, impression["video"], pImp);
        } else if(is_native) { //��Ϣ��
            if (SetImp_Native(impression, impression["native"], pImp) != 0) {
                return -1;
            }
        }
        SetImp_Pmp(impression["pmp"], pImp);
        rtb::Impression_Ext* pImExt = pImp->mutable_ext();//����Impression::Ext�ֶ�
        SetImp_Ext(impression["ext"], pImExt);
        rtb_request_->set_trace_id(CreateTraceId());//����һ��trace_id
        traceid_impression_[rtb_request_->trace_id()] = &imp[i];//������Ч�Ĺ��λӳ�䣬Ӧ��ʱʹ��
        return 0;//����λ�Ļ�ֻȡһ��
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
    //Youku�����У�δָ��banner����Ĵ���mime����
}

void YoukuJsonObject::SetImp_Video(const OpenDspJson::Value &imp,
                                   const OpenDspJson::Value &video,
                                   rtb::Impression *pImp)
{
    rtb::Video *pRtbVideo = pImp->mutable_video();
    //MIME���룬 Ŀǰ֧�֣� video/x-flv��application/x-shockwave-flash��
    //text/html:baner�Ķ�̬�ز�
    const OpenDspJson::Value &mimes = video["mimes"];
    for(OpenDspJson::Value::ArrayIndex i = 0; i < mimes.size(); ++i) {
        //֧�ֲ��ŵ���Ƶ��ʽ��Ŀǰ֧�֣� video/x-flv��application/x-shockwave-flash��
        SURE_CVALUE(pRtbVideo->add_formats, mimes[i]);
    }


    // "In-stream/linear" ָ���ǣ�Ϊ�˿���Ƶ���ݣ�ǿ���û�����ۿ���ǰ��/����/�������
    // ��Overlay/non-linear�� ָ���ǣ�����Ƶ������չʾ�Ĺ�档
    //���չ����ʽ set VideoLinearity
    //0: "δ֪";
    //1��"instream/linear"��������Ƭ�زģ�����ǰ����������;
    //2:"overlay/nonlinear"����Ƶ�����е��������;
    //3��"pause"����Ƶ������ͣ�е��������;
    //4:"fullscreen"����Ƶȫ������ʱ���������;
    int line = ASINT(video["linearity"]);
    if(rtb::VideoLinearity_IsValid(line)) {
        pRtbVideo->set_linearity((rtb::VideoLinearity)line);
    } else {
        LOG_WARN << "pid:" << imp["tagid"].asString() << " has new linearity:" << line;
    }

    // set min_duration ��Ƶ�����̲���ʱ������λ����
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
    // set start delay ���ֶν���linearity=1ʱ��Ч��������Ƭ��0��ǰ����-1��������-2��������
    // ��λ(��)
    // ���ֵ����0��������������ֵ��ʾ��Ƶ���Ŷ������Ժ�����
    // 0 -- ǰ��
    // 1 - ����
    // 2 - ����
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
    //��site��app���content���ݷ���rtb::impression::video::content�У����ٷֱ����
    //rtb::site��rtb::app���ˡ�Ҳ����˵�������ͬ��rtb::impression::video����ͬ��content����
    rtb::Content* pRtbVideoContent = pRtbVideo->mutable_ext()->mutable_content();
    const OpenDspJson::Value &content =
        ((!root_["site"].empty()) ? root_["site"]["content"] : root_["app"]["content"]);
    //site��appֻ��һ��content
    SetSiteApp_Content(content, pRtbVideoContent);
}

int YoukuJsonObject::SetImp_Native(const OpenDspJson::Value &imp,
                                    const OpenDspJson::Value &native,
                                    rtb::Impression *pImp)
{
    auto &native_template_ids = native["native_template_ids"];
    //֧�ֵ�ģ��id���ϣ�һ������棩λ��֧�ֶ��ģ����ʽ
    for(OpenDspJson::Value::ArrayIndex i = 0; i < native_template_ids.size(); ++i) {
        const char *id = native_template_ids[i].asCString();
        if (id && memcmp(id, "100", 3) == 0) {
            pImp->add_view_types(rtb::VT_YT_MOBILE_APP_NATIVE_100);
        } else {
            return -1;
        }
    }
    //��Ϣ���淶���������Ǳ߹�ͨ���ˣ��Ȳ���ᣬ�����Ϊ׼
    /*
    for(OpenDspJson::Value::ArrayIndex i = 0; i < assets.size(); ++i) {
        auto &asset = assets[i];
        std::string &native_template_id = asset["native_template_id"].asString();
        int len = ASINT(asset["title"]["len"]);//�����ⳤ������
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
        //deal �۸�,��λ�Ƿ�/ǧ���ع�,��CPM
        pDeal->set_bidfloor(ASINT(deal["bidfloor"]));
        // ��������
        // 1 -- first price auction.
        // 2 -- second price auction.
        // 3 -- the passed bidfloor indicates the apriori agreed upon deal price
        //���۵ķ�ʽ��Ŀǰ����1������һ���۷�����ߵ�deal��þ��۳ɹ���ȡ��߳�����Ϊ����ʤ���ۡ�
        //ע��ֻ�е����Dealͬʱ��Ӧʱ�Ż���֮�侺�ۡ�����ֶκ�OpenRTB��ͬ��
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
    //ý����վ����
    SURE_CVALUE(pSite->set_name, site["name"]);
    //��ǰҳ��URL
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
    //SetSiteApp_Content(content, pContent);///��Ϊ��video����ֵ
    return 0;
}

//������site����app����content�ṹ
void YoukuJsonObject::SetSiteApp_Content(const OpenDspJson::Value &content, rtb::Content *pContent)
{
    if(content.empty()) {
        return;
    }
    char *title = (char*)content["title"].asCString();
    if(title == NULL) {
        return;
    }
    //��Ƶ��������
    pContent->set_title(title);
    std::vector<std::string> vec;
    const std::string &keywords = content["keywords"].asString();
    //��Ƶ��ǩ�ؼ��֣�����Ƕ���ؼ��֣���ʹ��Ӣ�����߷ָ�
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
        //��Ƶ��Ƶ��ID������"a"�������Ƶ���б��μ��ֵ��ļ�Youku ADXƵ���б�
        if(!ext["channel"].empty()) {
            rtb::Content::Ext::Direct *direct = pExt->add_direct();
            direct->set_key("channel");
            direct->set_value(ext["channel"].asString());
        }

        if(!ext["cs"].empty()) {
            //����Ƶ��ID������Ķ���Ƶ���б���Ϣ���μ��ֵ��ļ�Youku ADX����Ƶ���б�
            rtb::Content::Ext::Direct *direct1 = pExt->add_direct();
            direct1->set_key("cs");
            direct1->set_value(ext["cs"].asString());
        }

        if(!ext["usr"].empty()) {
            //��Ƶ�ϴ���id���ſ����ϴ���Ƶ���û�ID
            rtb::Content::Ext::Direct *direct2 = pExt->add_direct();
            direct2->set_key("usr");
            direct2->set_value(ext["usr"].asString());
        }

        if(!ext["s"].empty()) {
            //��Ŀid
            rtb::Content::Ext::Direct *direct3 = pExt->add_direct();
            direct3->set_key("s");
            direct3->set_value(ext["s"].asString());
        }

        if(!ext["vid"].empty()) {
            //��Ƶid
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
    //pp�����ƣ�һ����"�ſ�ͻ���"������"�����ͻ���"��
    SURE_CVALUE(pApp->set_name, app["name"]);

    //��Ƶ�����������Ϣ��ֻ����Ƶ��Ƭ���͵Ĺ��λ�Ż�������ֶΣ�������site.content������ͬ
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

    //ʹ��MD5��ϣ��Device ID�����ſ���ƶ����������ֶ���IMEI��md5ֵ
    //pDevice->set_id(device["didmd5"].asString());//device_id
    //����ϵͳ
    //boost::to_lower(os);
    //std::transform(os.begin(), os.end(), os.begin(), tolower);
    SURE_CVALUE(pDevice->set_os, device["os"]);
    std::string *os = pDevice->mutable_os();
    os->length() > 0 ? boost::to_lower(*os) : void(0);
    //����ϵͳ�汾�ţ���"4.1", "XP"��
    SURE_CVALUE(pDevice->set_os_ver, device["osv"]);
    //�豸���ͣ���0���ֻ���1��ƽ�壬2��PC��3�����������ӡ�
    int devicetype = ASINT(device["devicetype"]);
    if(devicetype == 0) { //�ֻ�
        pDevice->set_device_type(DT_PHONE);
    } else if(devicetype == 1) { //ƽ��
        pDevice->set_device_type(DT_TABLET);
    } else if(devicetype == 2) { //PC
        pDevice->set_device_type(DT_PC);
    } else if(devicetype == 3) { //����������
        pDevice->set_device_type(DT_TV);
    }
    //��os = ��ios��ʱ��Ч, ���Ĵ��䣬Ĭ��Ϊ���ַ���
    if(!device["idfa"].empty() && *os == "ios") {
        const char *idfa = device["idfa"].asCString();
        pDevice->set_id(idfa);
        pDevice->set_ifa(idfa);
    } else {
        //��os = ��android��ʱ��Ч�����Ĵ��䣬Ĭ��Ϊ���ַ���
        //if(!device["androidid"].empty() && device["os"].asString() == "android")
        //IMEI, ���Ĵ��䣬Ĭ��Ϊ���ַ���
        SURE_CVALUE(pDevice->set_id, device["imei"]);
    }
    //MAC��ַ�����Ĵ��䣬Ĭ��Ϊ���ַ���
    if(!device["mac"].empty() && pDevice->id().length() == 0) {
        pDevice->set_id(device["mac"].asCString());
    }

    //���쳧��,�硰apple����Samsung����Huawei����Ĭ��Ϊ���ַ���
    SURE_CVALUE(pDevice->set_make, device["make"]);

    //�ͺ�, �硱iphoneA1530����Ĭ��Ϊ���ַ���
    SURE_CVALUE(pDevice->set_model, device["model"]);
    //��������, 0��δ֪; 1����̫��2��Wifi; 3���ƶ����� -δ֪; 4��2G; 5:3G
    int connectiontype = ASINT(device["connectiontype"]);
    // 0��δ֪; 1����̫��2��Wifi; 3���ƶ����� -δ֪; 4��2G; 5:3G
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


    //device["carrier"] :0��wifi��1���й��ƶ���2���й���ͨ��3���й����ţ�4��������5��δʶ��
    //device->carrier:1 unknown, 2 �й��ƶ�, 3 �й���ͨ, 4�й�����
    int carrier = ASINT(device["carrier"]);
    if(carrier == 1) {
        pDevice->set_carrier(2);    //�й��ƶ�
    } else if(carrier == 3) {
        pDevice->set_carrier(4);    //�й�����
    } else if(carrier == 2) {
        pDevice->set_carrier(3);    //�й���ͨ
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
    //�û�ID����PC�����У����ID��youku.com��cookie�е��û�ID�ֶΣ�
    //���ƶ������У����ֶξ���ȡֵ��ʽ���μ���¼UserIDȡֵ����
    SURE_CVALUE(pUser->set_id, user["id"]);

    //�Ա�"M"��ʾ���ԣ�"F"��ʾŮ�ԣ�Ϊ�ձ�ʾδ֪
    std::string m = user["gender"].asString();
    if(m == "M") {
        pUser->set_gender(rtb::GENDER_MALE);
    } else if(m == "F") {
        pUser->set_gender(rtb::GENDER_FEMALE);
    } else {
        pUser->set_gender(rtb::GENDER_UNKNOWN);
    }
    //������ݣ�Year Of Birth����4λ���֣���1988
    int birth = ASINT(user["yob"]);
    if(birth > 0) {
        pUser->set_year_of_birth(birth);
    }
    //�û���صı�ǩ��Ϣ��ֻ���ڰٶȣ�������user.tag
    const OpenDspJson::Value &tag = user["tag"];
    if(!tag.empty()) {
        //not imp
        /*
        ASINT(tag["1"]);//Ʒ���û��Ŀ�����
        ASINT(tag["2"]);//��Ϸ�û��Ŀ�����
        ASINT(tag["3"]);//�����û��Ŀ�����
        ASINT(tag["4"]);//��С��ҵ�û��Ŀ�����
        ASINT(tag["5"]);//�����û��Ŀ�����
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
    bid["id"] = rtb_bid.id();//DSP�Ըôγ��۷����ID
    bid["impid"] = (*imp)["id"];//�����ع�id
    bid["price"] = (double)rtb_bid.price();

    std::string strfeedback;
    BuildExpose(rtb_resp, &rtb_bid, "s=${AUCTION_PRICE}", strfeedback);
    bid["nurl"] = strfeedback;//win notice url ���۳ɹ�֪ͨ

    //����ز�URL������Ƕ�̬���⣬����ֶδ�ŵ��Ǵ����HTML��ǩ����ǩ��֧�����ֺ��滻��
    //%%CLICK_URL_ESC%%��Exchange�ĵ������ַ����%%WINNING_PRICE%%���������ռ۸�

    //RTB����ʱ��adm������Ƶurl(flv)��image_url����banner��url
    //banner���λ֧��gif��jpg��png��swf��flv��x��c�����زĸ�ʽ
    //video���λֻ֧��swf��flv��x�����زĸ�ʽ
    bid["adm"] = (!(*imp)["banner"].empty()) ? rtb_bid.image_url() : rtb_bid.adm();

    if(bid["adm"].asString().length() == 0 && (*imp)["native"].empty()) {
        LOG_WARN << "bid[\"adm\"] is null!";
        bid["adm"] = rtb_bid.image_url().length() > 0 ? rtb_bid.image_url() : rtb_bid.adm();
    }

    bid["crid"] = rtb_bid.creative_id();
    //DSP�μӵ�deal id
    if(!rtb_bid.deal_id().empty()) {
        bid["dealid"] = rtb_bid.deal_id();
    }

    OpenDspJson::Value &bid_ext = bid["ext"];
    //���Ŀ��URL
    bid_ext["ldp"] = rtb_bid.ext().click_url();//Ŀǰdest_url��click_urlֵ��ͬ
    //�ع���URL
    //OpenDspJson::Value feedback(strfeedback);
    //bid_ext["pm"].append(feedback);


    //std::string clickUrlEncode;
    std::string urlStr;
    //unsigned int crc;
    //std::string encodestr;
    //StringEncode(rtb_bid.ext().click_url(), crc, encodestr);
    //StringCodec::UrlEncode(encodestr, clickUrlEncode);
    //�����ת��ַ���ٴ��ݣ���ldp�ֶ�ִ�м���
    BuildClick(rtb_resp, &rtb_bid, "", urlStr);
    OpenDspJson::Value clickUrl(urlStr);
    bid_ext["cm"].append(clickUrl);//�������ַ
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
