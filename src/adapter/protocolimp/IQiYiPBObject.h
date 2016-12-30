#ifndef IQIYIPROTOBUFOBJECT_H
#define IQIYIPROTOBUFOBJECT_H

/*
auth:xxxx
date:2016/10
*/

#include "CommonPb.h"
#include "poseidon_iqiyi_req.pb.h"
#include "poseidon_iqiyi_rsp.pb.h"

namespace poseidon
{
namespace adapter
{

class IQiYiPBObject : public CommonPb
{
public:
    IQiYiPBObject();
    virtual ~IQiYiPBObject();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return iqiyi_resp_timeout_; }
    static void InitStaticVar();
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return iqiyi_request_.id().c_str(); }
private:
    static std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static rtb::TrafficSource source_;
    static double iqiyi_resp_timeout_;
    static std::map<int64_t, std::vector<int> > pid_viewtype_map_;
    static std::set<int> download_adtype_;//多ad_type情况下用来筛选下载type。目前未使用。
    static void ViewMap();

    char empty_resp_;//是否发送空应答
    uint16_t controler_resp_flag_;//位图，初始值为1。controler成功返回就左移一位
    ads_serving::proto::BidRequest &iqiyi_request_;
    ads_serving::proto::BidResponse &response_;
    //保存trace_id和imp之间的映射关系。应答组装包时需要用到
    std::map<std::string, const ads_serving::proto::Impression*> traceid_impid_map_;
    void BuildEmptyResp();
    void AdxResponse(const std::string &response);
    int SetUser();
    int SetSite();
    void SetContent(rtb::Content *pContent, const ads_serving::proto::Content &content);
    int SetDevice();
    int SetImpression(std::vector<RtbReqSharedPtr> &);
    void SetBanner(rtb::Impression *pImp, const ads_serving::proto::Banner &banner);
    void SetVideo(rtb::Impression *pImp, const ads_serving::proto::Video &video);
    void SetResponse(poseidon::rtb::BidResponse &rtb_response, ads_serving::proto::Bid *iqy_bid,
                                const rtb::Bid &rtb_bid, const ads_serving::proto::Impression*);
    void VideoResp(poseidon::rtb::BidResponse &rtb_response, ads_serving::proto::Bid *iqy_bid,
                                const rtb::Bid &rtb_bid, const ads_serving::proto::Impression*);
    class InterClass
    {
    public:
        explicit InterClass(IQiYiPBObject *parent) : parent_(parent) {}
        ~InterClass();
    private:
        IQiYiPBObject *parent_;
    };
    friend class InterClass;
};

}//adapter namespace
}//poseidon namespace
#endif // IQIYIPROTOBUFOBJECT_H
