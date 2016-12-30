#ifndef YOUTU_JSONOBJECT_H
#define YOUTU_JSONOBJECT_H

#include "JsonObject.h"

/*
auth:xxxx
date:2016-07
*/

namespace poseidon
{
namespace adapter
{

//由于jsoncpp转换整型asInt时，若数据类型非isInt()，则assert错误
//因此此宏先把字段转成安全的String类型，再转换成Int类型
//#define ASINT(OBJ) (atoi(OBJ.asCString()))

class YoukuJsonObject : public JsonObject
{
public:
    YoukuJsonObject();
    virtual ~YoukuJsonObject();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return youtu_resp_timeout_; }
    static void InitStaticVar(muduo::net::EventLoop*);
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return root_["id"].asCString(); }
private:
    //优酷广告位信息
    class Youtuadplacements
    {
     public:
        int adplacementid_;//广告位id
        std::string adplacementname_;//名称
        std::string size_;//大小 800*400
        int bidfloor_;//广告位底价，单位是：分/CPM
        std::set<std::string> blockcategory_;//广告主行业黑名单，这里的值是黑名单行业的行业ID
        std::set<std::string> allowmaterial_;//允许的素材类型，内容是后缀名。注意，优酷视频贴片素材在这里是用flv代表的
    };
    static std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static rtb::TrafficSource source_;
    static double youtu_resp_timeout_;
    static std::map<std::string, int> pid_viewtype_map_;
    //广告位id-class Youtuadplacements
    static std::map<int, Youtuadplacements> pid_object_map_;//暂时未使用

    static void ViewMap();
    static void MapPid(const std::string &pids, int view_type);
    static void GetAdPlacements(muduo::net::EventLoop*);
    ////////////////////////////////////////////////
    std::map<std::string, const OpenDspJson::Value *> traceid_impression_;
    int SetImp(const OpenDspJson::Value &imp);
    void SetImp_Banner(const OpenDspJson::Value &imp, const OpenDspJson::Value &banner, rtb::Impression *pImp);
    void SetImp_Video(const OpenDspJson::Value &imp,  const OpenDspJson::Value &video, rtb::Impression *pImp);
    int SetImp_Native(const OpenDspJson::Value &imp,  const OpenDspJson::Value &native, rtb::Impression *pImp);
    void SetImp_Pmp(const OpenDspJson::Value &pmp, rtb::Impression *pImp);
    void SetImp_Ext(const OpenDspJson::Value &ext, rtb::Impression_Ext* pImExt);

    int SetSite(const OpenDspJson::Value &site);
    void SetSiteApp_Content(const OpenDspJson::Value &content, rtb::Content *pContent);
    int SetApp(const OpenDspJson::Value &app);
    int SetDevice(const OpenDspJson::Value &device);
    int SetUser(const OpenDspJson::Value &user);

/////////////////////////////////////////////////
    void SetResponse(const rtb::BidResponse &,const rtb::Bid &rtb_bid, OpenDspJson::Value &bid);

/////////////////////////////////////////////////
    void YouKuSuccResponse(TcpConnectionWptr &wptr, const std::string &response);
    void YouKuFailedResponse(TcpConnectionWptr &wptr);
};


}
}
#endif // YOUTU_JSONOBJECT_H
