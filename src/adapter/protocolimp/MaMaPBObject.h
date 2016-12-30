#ifndef TANXPROTOBUFOBJECT_H
#define TANXPROTOBUFOBJECT_H

/*
auth:xxxx
date:2016/07
*/

#include "CommonPb.h"
#include "poseidon_tanx.pb.h"

namespace poseidon
{
namespace adapter
{
//alimama tanx接入对象
class MaMaPBObject : public CommonPb
{
public:
    friend class TanxAdapter;
    MaMaPBObject();
    virtual ~MaMaPBObject();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return mama_resp_timeout_; }
    tanx::BidRequest &mama_tanx_request_;
    static void InitStaticVar();
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return mama_tanx_request_.bid().c_str(); }
private:
    static std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static rtb::TrafficSource source_;
    static double mama_resp_timeout_;
    static std::map<std::string, int> pid_viewtype_map_;

    static void InitMapViewType();

    void BuildEmptyResp(int version, const std::string &bid, TcpConnectionWptr wptr);
    void AdxResponse(TcpConnectionWptr &wptr, const std::string &response, const std::string &bid);
};

}
}
#endif // TANXPROTOBUFOBJECT_H
