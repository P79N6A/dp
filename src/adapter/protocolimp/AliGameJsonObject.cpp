#include "AliGameJsonObject.h"
#include <vector>
#include <utility> //make_pair

#include "utility/StringCodec.h"
#include "utility/encrypt/mz_decode.h"
#include <boost/algorithm/string.hpp>

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;
using namespace OpenDspJson;
/*
���ں���ý���������⣺
1����SSP�Ŷӽ���ģ�������������appid
2��ֱ�ӽ���DSP�ģ����簮�ģ�������ֱ�ӷ���appid
3������2�����ֺŶ�������
*/


extern Configer * volatile configer;

std::string AliGameJsonObject::dspid_;
rtb::TrafficSource AliGameJsonObject::source_ = rtb::TS_ALIGAME;
std::string AliGameJsonObject::click_url_;
std::string AliGameJsonObject::expose_url_;
std::string AliGameJsonObject::downloaded_url_;
double AliGameJsonObject::aligame_resp_timeout_;
static std::map<std::string, std::string> device_key;

__thread char err_msg[256] = { 0 };
#define SET_ERR(x) strcpy(err_msg, x)
#define CLR_ERR err_msg[0]='\0'


AliGameJsonObject::AliGameJsonObject() : JsonObject()
{
    //ctor
}

AliGameJsonObject::~AliGameJsonObject()
{
    //dtor
    auto &bidseats = response_["seatbid"];
    if (!bidseats.empty() && bidseats.size() > 0) {
        LOG_INFO << "[<--Aligame Response-->]\n" << OpenDspJson::StyledWriter().write(response_);
        OpenDspJson::FastWriter writer;
        const std::string &json_str = writer.write(response_);
        AliGameSuccResponse(wptr_, json_str);
    } else {
        AliGameFailedResponse(wptr_);
    }
}

void AliGameJsonObject::OnThreadInitStatic()
{

}

void AliGameJsonObject::InitStaticVar()
{
    dspid_ = configer->GetProperty<std::string>("base.aligame_dspid", "");
    std::string host = configer->GetProperty<std::string>("base.fb_host", "");
    std::string feedback = configer->GetProperty<std::string>("base.aligame_feedback_url", "");
    click_url_ = host + feedback + "/click?";
    expose_url_ = host + feedback + "/feedback?";
    downloaded_url_ = host + feedback + "/download?";

    int timeout = configer->GetProperty<int>("net.aligame_response_timeout", 120);
    aligame_resp_timeout_ = (double)timeout / 1000;
    LOG_INFO << "aligame_resp_timeout:" << aligame_resp_timeout_;

    const auto &list = configer->GetSectionKeys("token");//�豸id����ʹ�á�
    std::copy(list.begin(), list.end(), std::inserter(device_key, device_key.end()));
}



bool AliGameJsonObject::ParseFromBuff(const HttpRequest*, const char *buff, size_t len)
{
    return reader_.parse(buff, buff + len, root_);
}

//��adxRequest����ʧ��ʱ�����еĻص�
void AliGameJsonObject::OnFailed()
{

}


int AliGameJsonObject::OnRequest(const HttpRequest *request, std::vector<RtbReqSharedPtr> &rtb_request_list)
{
    MON_ADD(ATTR_ADAPTER_ALIGAME_REQUEST, 1);
    OpenDspJson::Value &imp = root_["adzinfo"];
    OpenDspJson::Value &site = root_["site"];
    OpenDspJson::Value &app = root_["app"];
    OpenDspJson::Value &device = root_["device"];
    if(imp.empty() || device.empty()) {
        //SET_ERR("Invalid aligame Json object!");
        LOG_ERROR << "Invalid aligame Json object!";
        return -1;
    }

    if(site.empty() && app.empty()) {
        //SET_ERR("site and app both are empty!");
        LOG_ERROR << "site and app both are empty!";
        return -2;
    }

    if(imp.type() != OpenDspJson::arrayValue) {
        //SET_ERR("adzinfo must be a array type!");
        LOG_ERROR << "adzinfo must be a array type!";
        return -3;
    }

    /*
    ���ں���ý���������⣺
    1����SSP�Ŷӽ���ģ�������������appid
    2��ֱ�ӽ���DSP�ģ����簮�ģ�������ֱ�ӷ���appid
    3������2�����ֺŶ�������
    */
    app_id_ = root_["appid"].asString();
    rtb_request_->set_id(Id());
    try {
        int ret = SetImp(imp);
        if(ret != 0) {
            LOG_ERROR << "SetImp failed!";
            return ret;
        }


        if(!site.empty()) { //�������������(pc/�ƶ��豸)
            ret = SetSite(site);
            if(ret != 0) {
                LOG_ERROR << "SetSite failed!";
                return ret;
            }

        } else { //app��������
            ret = SetApp(app);
            if(ret != 0) {
                LOG_ERROR << "SetApp failed!";
                return ret;
            }
        }

        ret = SetDevice(device);
        if(ret != 0) {
            LOG_ERROR << "SetDevice failed!";
            return ret;
        }

    } catch(const std::exception &e) {
        //SET_ERR("invalid json protocol!");
        LOG_ERROR << "id:" << Id() << " aligame Request deal failed:" << e.what();
        return -1;
    }

    rtb_request_->mutable_ext()->set_dsp_id(app_id_/*dspid_*/);//���ǻ�����õ�dspid

    //if(app_id_ == "11000")//test temp TODO
    //{
    //    return -1;
    //}
    rtb_request_->set_trace_id(CreateTraceId());
    rtb_request_list.push_back(rtb_request_);
    LOG_DEBUG << "[-->Aligame Request<--]\n" << OpenDspJson::StyledWriter().write(root_) << "\n" <<
              "[<--Rtb Request-->]\n" << rtb_request_->DebugString();
    MON_ADD(ATTR_ADAPTER_CONTROLER_ALIGAME_REQUEST, rtb_request_list.size());

    return 0;
}


int AliGameJsonObject::RtbResponse(rtb::BidResponse &rtb_response, EventLoop *loop)
{
    MON_ADD(ATTR_ADAPTER_CONTROLER_ALIGAME_RESPONSE, 1);
    if(rtb_response.no_bid_reason() != 0) {
        //LOG_WARN << "no ads of bid from controller, reason code is:" << rtb_response.no_bid_reason();
        return -1;
    }

    response_["id"] = Id();//����ID

    if(rtb_response.bid_seats_size() == 0) {
        LOG_ERROR << "Rtb bid_seats_size = 0!";
        return -2;
    }

    rtb::BidSeat* pBidSeat = rtb_response.mutable_bid_seats(0);
    for (int j = 0; j < pBidSeat->bids_size(); ++j) {
        OpenDspJson::Value bid;//��Ե����ع�ĳ���
        rtb::Bid *pBid = pBidSeat->mutable_bids(j);
        SetResponse(rtb_response, pBid, bid, j);
        response_["seatbid"].append(bid);
    }

    return 0;
}



///////////////////////////Request adpater///////////////////////////////////
int AliGameJsonObject::SetImp(const OpenDspJson::Value &imp)
{
    for(OpenDspJson::Value::ArrayIndex i = 0; i < imp.size(); ++i) {
        const OpenDspJson::Value &impression = imp[i];

        int vt = ASINT(impression["viewtype"]);
        if(!rtb::ViewType_IsValid(vt)) {
            LOG_ERROR << "Invalid view_type:" << vt;
            return -1;
        }
        rtb::Impression *pImp = rtb_request_->add_impressions();
        SURE_CVALUE(pImp->set_id, impression["pid"]);//���λID
        pImp->mutable_ext()->set_view_type(vt);
        if(!impression["img"].empty()) {
            SetImp_Banner(impression, impression["img"], pImp, i);
        } else if(!impression["video"].empty()) {
            SetImp_Video(impression["video"], pImp);
        }
        rtb::Impression_Ext* pImExt = pImp->mutable_ext();//����Impression::Ext�ֶ�
        pImExt->set_ad_num(ASINT(impression["adnum"]));
        //mixer
        const OpenDspJson::Value &mixer = impression["mixer"];
        if(!mixer.empty()) {
            Imp_Mixer(mixer, pImExt);
        }
    }
    return 0;
}

//���ģʽ
void AliGameJsonObject::Imp_Mixer(const OpenDspJson::Value &mixer, rtb::Impression_Ext* pImExt)
{
    rtb::App_Ext *pApp_ext = rtb_request_->mutable_app()->mutable_ext();
    const OpenDspJson::Value &templateids = mixer["templateid"];
    if(templateids.type() != OpenDspJson::arrayValue) {
        //SET_ERR("adzinfo must be a array type!");
        LOG_ERROR << "templateid must be a array type!";
        return;
    }

    for(OpenDspJson::Value::ArrayIndex i = 0; i < templateids.size(); ++i) {
        pApp_ext->add_native_template_ids(ASINT(templateids[i]));
    }
}

void AliGameJsonObject::SetImp_Banner(const OpenDspJson::Value &imp,
                                      const OpenDspJson::Value &banner,
                                      rtb::Impression *pImp,
                                      int idx)
{
    rtb::Banner* pBan = pImp->mutable_banner();
    char id[8];
    snprintf(id, sizeof(id), "%d", idx);
    pBan->set_id(id);
    pBan->set_width(ASINT(banner["w"]));
    pBan->set_height(ASINT(banner["h"]));
    //pBan->set_position(ASINT(banner["pos"]));
    for(OpenDspJson::Value::ArrayIndex i = 0; i < banner["formats"].size(); ++i) {
        SURE_CVALUE(pBan->add_formats, banner["formats"]);
    }

    //adzinfo������viewtype���ͣ������´���ע��
    /* if(ASINT(root_["device"]["devicetype"]) == 2)
         pImp->mutable_ext()->set_view_type(VT_POP_WINDOW);
     else
         pImp->mutable_ext()->set_view_type(VT_WL_BANNER);
    */
    //Youku�����У�δָ��banner����Ĵ���mime����
}

void AliGameJsonObject::SetImp_Video(const OpenDspJson::Value &video, rtb::Impression *pImp)
{
    rtb::Video *pRtbVideo = pImp->mutable_video();
    //MIME���룬 Ŀǰ֧�֣� video/x-flv��application/x-shockwave-flash��
    const OpenDspJson::Value &formats = video["formats"];
    for(OpenDspJson::Value::ArrayIndex i = 0; i < formats.size(); ++i) {
        //֧�ֲ��ŵ���Ƶ��ʽ��Ŀǰ֧�֣� video/x-flv��application/x-shockwave-flash��
        SURE_CVALUE(pRtbVideo->add_formats, formats[i]);
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

}



int AliGameJsonObject::SetSite(const OpenDspJson::Value &site)
{
    rtb::Site *pSite = rtb_request_->mutable_site();

    //pSite->set_site_id(vPid[2]);
    //ý����վ����
    SURE_CVALUE(pSite->set_name, site["title"]);
    //��ǰҳ��URL
    SURE_CVALUE(pSite->set_page, site["url"]);
    //Referrer URL
    SURE_CVALUE(pSite->set_ref, site["ref"]);
    return 0;
}


int AliGameJsonObject::SetApp(const OpenDspJson::Value &app)
{
    if(app.empty()) {
        return -1;
    }
    rtb::App *pApp = rtb_request_->mutable_app();
    //pp�����ƣ�һ����"�ſ�ͻ���"������"�����ͻ���"��

    SURE_CVALUE(pApp->set_name, app["name"]);
    SURE_CVALUE(pApp->set_version, app["version"]);
    SURE_CVALUE(pApp->set_bundle, app["bundle"]);

    const OpenDspJson::Value &keywords = app["keywords"];
    for(OpenDspJson::Value::ArrayIndex i = 0; i < keywords.size(); ++i) {
        SURE_CVALUE(pApp->add_keywords, keywords[i]);
    }

    OpenDspJson::Value::Members keys = app["ext"].getMemberNames();
    rtb::Content_Ext *ext = pApp->mutable_content()->mutable_ext();
    for(size_t i = 0; i < keys.size(); ++i) {
        const std::string &key = keys[i];
        rtb::Content_Ext_Direct *direct = ext->add_direct();
        direct->set_key(key);
        direct->set_value(app["ext"][key].asString());
    }

    return 0;
}


int AliGameJsonObject::SetDevice(const OpenDspJson::Value &device)
{
    if(device.empty()) {
        return -1;
    }
    rtb::Device* pDevice = rtb_request_->mutable_device();
    SURE_CVALUE(pDevice->set_ip, device["ip"]);
    //user agent
    SURE_CVALUE(pDevice->set_user_agent, device["ua"]);

    //pDevice->set_id(device["didmd5"].asString());//device_id
    //����ϵͳ
    if(!device["os"].empty()) {
        SURE_CVALUE(pDevice->set_os, device["os"]);
        std::string *os = pDevice->mutable_os();
        boost::to_lower(*os);
    }

    //����ϵͳ�汾�ţ���"4.1", "XP"��
    SURE_CVALUE(pDevice->set_os_ver, device["osv"]);
    //�豸���ͣ���0���ֻ���1��ƽ��
    int devicetype = ASINT(device["devicetype"]);
    if(devicetype == 0) { //�ֻ�
        pDevice->set_device_type(DT_PHONE);
    } else if(devicetype == 1) { //ƽ��
        pDevice->set_device_type(DT_TABLET);
    }


    std::string dev_id;
    if(!device["deviceid"].empty()) {
        const char *encode_str = device["deviceid"].asCString();
        auto iter = device_key.find(app_id_);//aligameЭ�����ͬappid�в�ͬ��token�������豸��
        if(iter == device_key.end()) { //�����Ҳ�������Ĭ��ʹ�ð�����Ϸ��token
            iter = device_key.find("11000");
        }

        if(iter != device_key.end()) {
            std::string &key = iter->second;
            dev_id = mz_decode(key.c_str(), encode_str);
        }
    }
    pDevice->set_id(dev_id);

    //��os = ��ios��ʱ��Ч
    if(pDevice->os() == "ios") {
        pDevice->set_ifa(dev_id);
    }

    //���쳧��,�硰apple����Samsung����Huawei����Ĭ��Ϊ���ַ���
    SURE_CVALUE(pDevice->set_make, device["brand"]);

    //�ͺ�, �硱iphoneA1530����Ĭ��Ϊ���ַ���

    SURE_CVALUE(pDevice->set_model, device["model"]);
    int connectiontype = ASINT(device["net"]);
    // 0 - Unknown��1 - wifi��2 - 2g��3 - 3g��4 - 4g
    if(connectiontype == 0) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_UNKNOWN);
    } else if(connectiontype == 1) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_WIFI);
    } else if(connectiontype == 2) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_2G);
    } else if(connectiontype == 3) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_3G);
    } else if(connectiontype == 4) {
        pDevice->set_connection_type(rtb::CONNECTION_TYPE_CELLULAR_DATA_4G);
    }


    int w = ASINT(device["res_w"]);
    int h = ASINT(device["res_h"]);
    if(w > 0 && h > 0) {
        char size[32];
        snprintf(size, sizeof(size), "%dx%d", w, h);
        pDevice->mutable_ext()->set_dev_resolution(size);

    }

    return 0;
}


////////////////////////////////////////////////////////////////////////////////////
void AliGameJsonObject::SetResponse(const rtb::BidResponse &rtb_resp, const rtb::Bid *pBid, OpenDspJson::Value &bid, int idx)
{
    bid["impid"] = pBid->impid();//	Bid Request�ж�Ӧ�Ĺ��λID
    if(idx >= root_["adzinfo"].size()) {
        idx = 0;
    }

    std::string strfeedback;
    BuildExpose(rtb_resp, pBid, "", strfeedback);
    //bid["nurl"] = strfeedback;//win notice url ���۳ɹ�֪ͨ
    bid["feedback"] = strfeedback;

    if(pBid->has_image_url() && pBid->image_url().length() > 0) { //�����زĲ���image_url��dst_url���ʼ���if�жϡ�
        bid["htmlsnippet"] = pBid->image_url();    //pBid->adm()
    }
    if(pBid->ext().has_dest_url() && pBid->ext().dest_url().length() > 0) {
        bid["dest_url"] = pBid->ext().dest_url();    //���ҳ��ַ
    }

    //std::string clickUrlEncode;
    std::string urlStr;
    //unsigned int crc;
    //std::string encodestr;
    //StringEncode(pBid->ext().click_url(), crc, encodestr);
    //StringCodec::UrlEncode(encodestr, clickUrlEncode);
    //clickUrlEncode = "u=" + clickUrlEncode;
    //if(app_id_ == "11000")
    BuildClick(rtb_resp, pBid, "u=", urlStr);
    //else
    //    BuildClick(rtb_resp, pBid, clickUrlEncode, urlStr);
    OpenDspJson::Value clickUrl(urlStr);
    bid["click"] = clickUrl;//�������ַ

    std::string download_complete_url;
    BuildDownloadcomplete(rtb_resp, pBid, download_complete_url);
    bid["download_complete_url"] = download_complete_url;

    //TODO:judge view_type=301
    const std::string &json = pBid->specific_data();
    OpenDspJson::Value specific;
    //�����±��ȡ��������Ϣ��Ӧ��view_type
    int view_type = ASINT(root_["adzinfo"][idx]["viewtype"]);
    if(view_type == rtb::VT_WL_MIX_APP) {
        if(!OpenDspJson::Reader().parse(json, specific)) {
            LOG_ERROR << "�Ƿ���mixer json����ֵ��";
            return;
        }
        OpenDspJson::Value &bid_mixers = specific["mixers"];
        if(bid_mixers.type() != OpenDspJson::arrayValue) {
            LOG_ERROR << "bid_mixers must be a array type!";
            return;
        }
        for(OpenDspJson::Value::ArrayIndex i = 0; i < bid_mixers.size(); ++i) {
            OpenDspJson::Value mixer;
            OpenDspJson::Value &bid_mixer = bid_mixers[i];
            mixer["templateid"] = ASINT(bid_mixer["templateid"]);
            mixer["video_url"] = bid_mixer["video_url"].asString();
            mixer["end_img_url"] = bid_mixer["end_img_url"].asString();
            mixer["download_url"] = bid_mixer["dest_url"].asString();
            bid["mixers"].append(mixer);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////


void AliGameJsonObject::AliGameFailedResponse(TcpConnectionWptr &wptr)
{
    static const string simple_resp = "HTTP/1.1 204 No Content\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      "Content-Type: application/json\r\n\r\n";

    MON_ADD(ATTR_ADAPTER_ALIGAME_EMPTY_RESPONSE, 1);
    //LOG_INFO << "OUT-id_9You_empty_resp:" << Id();
    muduo::net::TcpConnectionPtr ptr = wptr.lock();
    if(ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(simple_resp.c_str(), simple_resp.length());
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_ALIGAME_RESPON_NOT_CONNECTD, 1);
    }
}

void AliGameJsonObject::AliGameSuccResponse(TcpConnectionWptr &wptr, const std::string &response)
{
    static const string simple_resp = "HTTP/1.1 200 OK\r\n"
                                      //"Connection: Keep-Alive\r\n"
                                      "Content-Type: application/json\r\n"
                                      "Content-length:";
    MON_ADD(ATTR_ADAPTER_ALIGAME_SUCC_RESPONSE, 1);
    //LOG_INFO << "OUT-id_9You_resp:" << Id();
    LogStream ss;
    ss << simple_resp << response.length() << "\r\n\r\n";
    ss << response;
    muduo::net::TcpConnectionPtr ptr = wptr.lock();
    if (ptr) {
        MON_ADD(ATTR_ADAPTER_ADX_RESPON_TOTAL, 1);
        ptr->send(ss.buffer().data(), ss.buffer().length());
    } else {
        //LOG_WARN << Id() << ":Connection Closed!";
        MON_ADD(ATTR_ADAPTER_ALIGAME_RESPON_NOT_CONNECTD, 1);
    }
}
