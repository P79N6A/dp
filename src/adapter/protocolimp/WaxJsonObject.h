#ifndef WAX_JSONOBJECT_H
#define WAX_JSONOBJECT_H

#include "JsonObject.h"

/*
auth:xxxx
date:2016-07
desc:微博粉丝通适配
*/

namespace poseidon
{
namespace adapter
{


class WaxJsonObject : public JsonObject
{
public:
    WaxJsonObject();
    virtual ~WaxJsonObject();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return wax_resp_timeout_; }
    static void InitStaticVar();
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual std::string& GetWinNoticeUrl() { return winnotice_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return root_["id"].asCString(); }//若空请求，此代码会raise
private:
    static std::string dspid_;
    static std::string winnotice_url_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static rtb::TrafficSource source_;
    static double wax_resp_timeout_;
    ////////////////////////////////////////////////
    uint32_t empty_response_flag_;
    //std::vector<int> view_type_vec_;
    std::map<std::string, const OpenDspJson::Value*> traceid_impression_map_;
    int SetImp(const OpenDspJson::Value &imp, std::vector<RtbReqSharedPtr>&);
    void SetImp_Banner(const OpenDspJson::Value &imp, const OpenDspJson::Value &banner, rtb::Impression *pImp);
    void SetImp_Video(const OpenDspJson::Value &imp,  const OpenDspJson::Value &video, rtb::Impression *pImp);
    void SetImp_Pmp(const OpenDspJson::Value &imp, rtb::Impression *pImp);
    void SetImp_Ext(const OpenDspJson::Value &ext, rtb::Impression_Ext* pImExt);

    int SetApp(const OpenDspJson::Value &app);
    int SetDevice(const OpenDspJson::Value &device);
    int SetUser(const OpenDspJson::Value &user);

/////////////////////////////////////////////////
    void SetResponse(const rtb::BidResponse &,const rtb::Bid *pBid, OpenDspJson::Value &bid, const OpenDspJson::Value &);

/////////////////////////////////////////////////
    void WaxSuccResponse(const std::string &response);
    void WaxFailedResponse();
};


}
}
#endif // WAX_JSONOBJECT_H
