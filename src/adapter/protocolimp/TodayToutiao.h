#ifndef TODAYTOUTIAOPROTOBUFOBJECT_H
#define TODAYTOUTIAOPROTOBUFOBJECT_H

/*
auth:xxxx
date:2016/10
*/

#include "CommonPb.h"
#include "poseidon_toutiao.pb.h"

namespace poseidon
{
namespace adapter
{

class TodayToutiao : public CommonPb
{
public:
    TodayToutiao();
    virtual ~TodayToutiao();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return toutiao_resp_timeout_; }
    static void InitStaticVar();
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return request_.request_id().c_str(); }
private:
    static std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static std::string special_view_url_;
    static rtb::TrafficSource source_;
    static double toutiao_resp_timeout_;
    static std::map<int/*adtype*/, int/*view_type*/> adtype_viewtype_map_;
    //根据viewtype反查ad_type
    static std::map<int/*view_type*/, int/*ad_type*/> viewtype_adtype_map_;
    static void InitMap();

    char empty_resp_;//是否发送空应答
    toutiao_ssp::api::BidRequest &request_;
    toutiao_ssp::api::BidResponse &response_;
    //保存trace_id和imp之间的映射关系。应答组装包时需用到
    std::map<std::string, const toutiao_ssp::api::AdSlot*> traceid_impid_map_;
    void BuildEmptyResp();
    void AdxResponse(const std::string &response);
    int SetUser();
    int SetApp();
    void SetContent(rtb::Content *pContent, const toutiao_ssp::api::Content &content);
    int SetDevice();
    int SetImpression(std::vector<RtbReqSharedPtr> &);
    void SetImp_Pmp(const toutiao_ssp::api::Pmp &pmp, rtb::Impression *pImp);
    void SetBanner(rtb::Impression *pImp, const toutiao_ssp::api::AdSlot_Banner &banner);
    void SetResponse(poseidon::rtb::BidResponse &rtb_response, toutiao_ssp::api::Bid *iqy_bid,
                                const rtb::Bid &rtb_bid, const toutiao_ssp::api::AdSlot*);

    class InterClass
    {
    public:
        explicit InterClass(TodayToutiao *parent) : parent_(parent) {}
        ~InterClass();
    private:
        TodayToutiao *parent_;
    };
    friend class InterClass;
};

}//adapter namespace
}//poseidon namespace
#endif // TODAYTOUTIAOPROTOBUFOBJECT_H
