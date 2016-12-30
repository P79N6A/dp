#ifndef SOHUPROTOBUFOBJECT_H
#define SOHUPROTOBUFOBJECT_H

/*
auth:xxxx
date:2016/12
*/

#include "CommonPb.h"
#include "poseidon_sohu.pb.h"

namespace poseidon
{

namespace adapter
{

class InterClass;
class SohuPbObject : public CommonPb
{
public:
    SohuPbObject();
    virtual ~SohuPbObject();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return sohu_resp_timeout_; }
    static void InitStaticVar();
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return request_.bidid().c_str(); }
private:
    friend class InterClass;
    static std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static std::string special_view_url_;
    static rtb::TrafficSource source_;
    static double sohu_resp_timeout_;
    static std::map<std::string/*adtype*/, int/*view_type*/> adtype_viewtype_map_;
    //根据viewtype反查ad_type
    //static std::map<int/*view_type*/, int/*ad_type*/> viewtype_adtype_map_;
    static void InitMap();

    sohuadx::Request &request_;
    sohuadx::Response &response_;
    //保存trace_id和imp之间的映射关系。应答组装包时需用到
    std::map<std::string, const sohuadx::Request_Impression*> traceid_impid_map_;
    void AdxResponse(const std::string &response);
    void SetSite();
    int SetUser();
    int SetDevice();
    int SetImpression(std::vector<RtbReqSharedPtr> &);
    void SetBanner(rtb::Impression *pImp, const sohuadx::Request_Impression_Banner &banner);
    void SetVideo(rtb::Impression *pImp, const sohuadx::Request_Impression_Video &video);
    void SetResponse(poseidon::rtb::BidResponse &rtb_response, sohuadx::Response_Bid *iqy_bid,
                                const rtb::Bid &rtb_bid, const sohuadx::Request_Impression*);
};

}//adapter namespace
}//poseidon namespace
#endif // SOHUPROTOBUFOBJECT_H
