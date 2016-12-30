#ifndef ALIGAME_JSONOBJECT_H
#define ALIGAME_JSONOBJECT_H

#include "JsonObject.h"


/*
auth:xxxx
date:2016-07
*/

namespace poseidon
{
namespace adapter
{



class AliGameJsonObject : public JsonObject
{
public:
    AliGameJsonObject();
    virtual ~AliGameJsonObject();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);

    //youku adx timeoutÉèÎª150ms
    virtual double ResopnseTimeOut() { return aligame_resp_timeout_; }
    static void InitStaticVar();
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return root_["bid"].asCString(); }
    //virtual void buildFeedBackField(const rtb::Bid* pBid, int action_type, std::string& ret_str);
private:
    std::string app_id_;

    static std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static rtb::TrafficSource source_;
    static double aligame_resp_timeout_;
    ////////////////////////////////////////////////
    int SetImp(const OpenDspJson::Value &imp);
    void SetImp_Banner(const OpenDspJson::Value &imp, const OpenDspJson::Value &banner, rtb::Impression *pImp, int);
    void SetImp_Video(const OpenDspJson::Value &video, rtb::Impression *pImp);
    void Imp_Mixer(const OpenDspJson::Value &mixer, rtb::Impression_Ext* pImExt);

    int SetSite(const OpenDspJson::Value &site);
    int SetApp(const OpenDspJson::Value &app);
    int SetDevice(const OpenDspJson::Value &device);
/////////////////////////////////////////////////
    void SetResponse(const rtb::BidResponse &rtb_resp, const rtb::Bid *pBid, OpenDspJson::Value &bid, int idx);

/////////////////////////////////////////////////
    void AliGameSuccResponse(TcpConnectionWptr &wptr, const std::string &response);
    void AliGameFailedResponse(TcpConnectionWptr &wptr);

};

}
}
#endif // JSONOBJECT_H
