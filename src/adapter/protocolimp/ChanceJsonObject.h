#ifndef CHANCE_JSONOBJECT_H
#define CHANCE_JSONOBJECT_H

#include "JsonObject.h"


/*
auth:xxxx
date:2016-07
*/

namespace poseidon
{
namespace adapter
{

class ChanceJsonObject : public JsonObject
{
public:
    ChanceJsonObject();
    virtual ~ChanceJsonObject();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return chance_resp_timeout_; }
    static void InitStaticVar();
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return root_["sid"].asCString(); }
private:
    std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static rtb::TrafficSource source_;
    static double chance_resp_timeout_;
    ////////////////////////////////////////////////
    int SetImp(const OpenDspJson::Value &imp);
    int SetApp(const OpenDspJson::Value &app);
    int SetDevice(const OpenDspJson::Value &device);
/////////////////////////////////////////////////
    void SetResponse(const rtb::BidResponse &rtb_resp, const rtb::Bid *pBid, OpenDspJson::Value &bid);

/////////////////////////////////////////////////
    void ChanceSuccResponse(TcpConnectionWptr &wptr, const std::string &response);
    void ChanceFailedResponse(TcpConnectionWptr &wptr);

};

}
}
#endif // CHANCE_JSONOBJECT_H
