#ifndef YUNOSPBOBJECT_H
#define YUNOSPBOBJECT_H

#include "CommonPb.h"
#include "poseidon_yunos.pb.h"

namespace poseidon
{
namespace adapter
{

class YunOsPbObject : public CommonPb
{
public:
    YunOsPbObject();
    virtual ~YunOsPbObject();
    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return yunos_resp_timeout_; }
    static void InitStaticVar();
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return request_.bid().c_str(); }
private:
    static std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static rtb::TrafficSource source_;
    static std::string yunos_secret_;
    static double yunos_resp_timeout_;

    int send_empty_resp_;
    com::yunos::exchange::service::provider::dto::BidRequest &request_;
    com::yunos::exchange::service::provider::dto::BidResponse &response_;
    //key:trace_id-adid
    std::map<std::string, std::string> adid_list_;

    void Response(const std::string &response);
    int AuthEntry();

    class RtbResponFinal
    {
    public:
        RtbResponFinal(YunOsPbObject &parent, const rtb::BidResponse &rtsp) : parent_(parent),
                                                                             rtb_resp_(rtsp)
        {

        }
        ~RtbResponFinal();
    private:
        YunOsPbObject &parent_;
        const rtb::BidResponse &rtb_resp_;
    };
    friend class RtbResponFinal;
};

}
}

#endif // YUNOSPBOBJECT_H
