#ifndef TANXADAPTER_H
#define TANXADAPTER_H

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <muduo/net/InetAddress.h>

#include "common/common.h"
#include "net/HttpRequest.h"
#include "net/EventLoopContext.h"
#include "poseidon_tanx.pb.h"
#include "MaMaPBObject.h"

/**
    auth: xxxx
    date: 2016/05
**/
namespace poseidon
{

namespace adapter
{

class HttpRequest;
class UdpChannel;


struct SFItem
{
    std::string name;
    std::size_t begin;
    std::size_t end;
};

//TanxAdapterÊµÀý
class TanxAdapter : boost::noncopyable
{
public:
    TanxAdapter(MaMaPBObject*);
    virtual ~TanxAdapter();
    int Process(tanx::BidRequest &, RtbReqSharedPtr&, LoopContextPtr&);
    int ProcessToTanxResp(rtb::BidResponse &, std::string &content);
    static int buildEmptyTanxResp(int, const std::string &, std::string &);
private:
    MaMaPBObject *mama_pb_object_;
    /////////////////tanx request///////////////////
    int RtbAdzInfo(int, tanx::BidRequest_AdzInfo*, rtb::Impression*, tanx::BidRequest &);
    int RtbMobileInfo(tanx::BidRequest &request, rtb::App *, std::string&);
    int RtbSiteInfo(tanx::BidRequest &request, rtb::Site*, const std::string&, const std::string &tanxid);
    int SetOtherDevice(rtb::Device *pDev, tanx::BidRequest &request, LoopContextPtr &loopctx);

    int SetVideoAds(tanx::BidRequest_AdzInfo *pAdz,
                            rtb::Impression *pImp,
                            tanx::BidRequest &request);
    int SetBanner(tanx::BidRequest_AdzInfo *pAdz, rtb::Impression *pImp);
    int SetPmP(int, tanx::BidRequest_AdzInfo *,
                        rtb::Impression *,
                        tanx::BidRequest &);
    int SetExcludSenCateg(rtb::Impression_Ext *pImExt, tanx::BidRequest &request);
    int SetExcludAdCateg(rtb::Impression_Ext *pImExt, tanx::BidRequest &request);

    int buildPingResp(tanx::BidRequest &, std::string &);
    int buildTestResp(tanx::BidRequest &, std::string &);
    //////////////////dsp response////////////////////
    bool buildMobileCreative(const rtb::BidResponse &, rtb::Bid* pBid, tanx::BidResponse_Ads* pTanxAds,
                            const std::string& jsonContent);
    int BuildVideoSnippet(std::string& snippet, const std::string &feeback_url,
                                        rtb::Bid* pVideoBid, tanx::BidResponse_Ads* pVideoTanxAds);
    bool buildTrdUrl(rtb::Bid* pBid, std::string& trd);

    std::string GetDevice_type();
};


#define MARK_SYMBOL "%%"

class StringFinder
{
public:
    static void find(const std::string& destStr, std::vector<struct SFItem>& outVec) {
        size_t pos = 0;
        while ((pos = destStr.find(MARK_SYMBOL, pos)) != std::string::npos)
        {
            size_t s = pos;
            size_t e = 0;
            if ((pos = destStr.find(MARK_SYMBOL, pos+2)) != std::string::npos)
            {
                e = (pos += 2);
            }
            else
            {
                return;
            }
            SFItem data;
            data.name.assign(destStr, s, e - s);
            data.begin = s;
            data.end = e;
            outVec.push_back(data);
        }
    }
};

}
}

#endif // TANXADAPTER_H
