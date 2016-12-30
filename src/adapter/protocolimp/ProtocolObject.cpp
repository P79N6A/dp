#include "ProtocolObject.h"
#include "utility/StringCodec.h"

#include <sys/time.h>
#include <zlib.h>
#include <muduo/base/LogStream.h>
#include <muduo/base/CurrentThread.h>
#include <monitor_api.h> //MON_ADD
#include "utility/md5.h"
#include "poseidon_expose_url.pb.h"

using namespace poseidon;
using namespace poseidon::adapter;
using namespace muduo;
using namespace muduo::net;


namespace
{
__thread uint64_t sequence_id = 1;
__thread char trace_id_md5_buff[33];
//减少new次数
DECLARE_THREAD_VAR_LIST(rtb::BidRequest, rtb_req);
DECLARE_THREAD_VAR(poseidon::adapter::expose_url::UrlCoding, urlencode);
}

boost::function<void(rtb::BidRequest *)> ProtocolObject::dx_;


ProtocolObject::ProtocolObject() : rtb_request_(Alloc_rtb_req_shared())
{
    //ctor
    recv_time_ = GetMS();
}

ProtocolObject::~ProtocolObject()
{
    //dtor
    int64_t latecy_time = GetMS() - recv_time_;
    if(latecy_time < 10) {
        MON_ADD(ATTR_ADAPTER_LATECY_LESS10, 1);
    } else if(latecy_time < 30) {
        MON_ADD(ATTR_ADAPTER_LATECY_LESS30, 1);
    } else if(latecy_time < 50) {
        MON_ADD(ATTR_ADAPTER_LATECY_LESS50, 1);
    } else if(latecy_time < 100) {
        MON_ADD(ATTR_ADAPTER_LATECY_LESS100, 1);
    } else {
        MON_ADD(ATTR_ADAPTER_LATECY_GREAT100, 1);
    }
}

void ProtocolObject::InitStaticVar()
{
    LOG_INFO << "InitStaticVar";
    dx_ = boost::bind(&ProtocolObject::Destroy_rtb_req, _1);
}

void ProtocolObject::OnThreadInitStatic()
{
    //初始化线程变量
    InitThreadedQueuePool(THREAD_VAR_LIST(rtb_req));
    if (urlencode == NULL) {
        urlencode = new poseidon::adapter::expose_url::UrlCoding;
    }
}

void ProtocolObject::Init()
{
    rtb_request_->mutable_ext()->set_traffic_source(GetSource());
    rtb_request_->mutable_ext()->set_dsp_id(DspID());//dspid在InitStaticVar中初始化一次
    rtb_request_->set_version(RTB_VERSION);
}

RtbReqSharedPtr ProtocolObject::Alloc_rtb_req_shared()
{
    RtbReqSharedPtr result;
    result.reset(Alloc_rtb_req(), dx_);
    return result;
}

rtb::BidRequest* ProtocolObject::Alloc_rtb_req()
{
    return Alloc_Bid_Obj(THREAD_VAR_LIST(rtb_req));
}

void ProtocolObject::Destroy_rtb_req(rtb::BidRequest *req)
{
    DestroyObject(req, THREAD_VAR_LIST(rtb_req));
}

//virtual buildFeedBackField = 0;

void ProtocolObject::BidEncode(muduo::LogStream &ss)
{
    const std::string &bid = Id();
    std::string bid_encode;
    StringCodec::UrlEncode(bid, bid_encode);
    ss << "bid=" << bid_encode << "&";
}

void ProtocolObject::BuildClick(const rtb::BidResponse &rtb_resp,
                                const rtb::Bid* pBid,
                                const std::string &params,
                                std::string &urlStr)
{
    LogStream ss;
    unsigned int k;
    std::string str;
    std::string pbBase64Urlencode;
    ss << GetClickUrl();
    if(params.length() > 0) {
        ss << params << "&";
    }

    BidEncode(ss);

    buildFeedBackField(rtb_resp, pBid, 2, str);
    StringEncode(str, k, pbBase64Urlencode);
    ss << "c=" << k <<"&q=" << pbBase64Urlencode;
    urlStr.assign(ss.buffer().data(), ss.buffer().length());
}

void ProtocolObject::CustomBuildUrl(const rtb::BidResponse &rtb_resp,
                                    const rtb::Bid* pBid,
                                    const std::string &host_url,
                                    const std::string &params,
                                    int action_type,
                                    std::string &out)
{
    LogStream ss;
    unsigned int k;
    std::string str;
    std::string pbBase64Urlencode;
    ss << host_url; //TODO:
    if(params.length() > 0) {
        ss << params << "&";
    }

    BidEncode(ss);
    buildFeedBackField(rtb_resp, pBid, action_type, str);
    StringEncode(str, k, pbBase64Urlencode);
    ss << "c=" << k <<"&q=" << pbBase64Urlencode;
    out.assign(ss.buffer().data(), ss.buffer().length());
}


void ProtocolObject::BuildWinNotice(const rtb::BidResponse &rtb_resp,
                                    const rtb::Bid* pBid,
                                    const std::string &params,
                                    std::string &urlStr)
{
    LogStream ss;
    unsigned int k;
    std::string str;
    std::string pbBase64Urlencode;
    ss << GetWinNoticeUrl();
    if(params.length() > 0) {
        ss << params << "&";
    }

    BidEncode(ss);
    //action:1 展现, 2 点击, 3 下载 4 安装完成 0 winnotice
    buildFeedBackField(rtb_resp, pBid, 0, str);
    StringEncode(str, k, pbBase64Urlencode);
    ss << "c=" << k <<"&q=" << pbBase64Urlencode;
    urlStr.assign(ss.buffer().data(), ss.buffer().length());
}

void ProtocolObject::BuildExpose(const rtb::BidResponse &rtb_resp,
                                 const rtb::Bid* pBid,
                                 const std::string &params,
                                 std::string& snippet)
{
    LogStream ss;
    unsigned int k;
    std::string str;
    std::string pbBase64Urlencode;
    ss << GetExposeUrl();
    if(params.length() > 0) {
        ss << params << "&";
    }

    BidEncode(ss);

    buildFeedBackField(rtb_resp, pBid, 1, str);
    StringEncode(str, k, pbBase64Urlencode);
    ss << "c=" << k <<"&q=" << pbBase64Urlencode;
    snippet.assign(ss.buffer().data(), ss.buffer().length());
}

void ProtocolObject::BuildDownloadcomplete(const rtb::BidResponse &rtb_resp,
        const rtb::Bid* pBid,
        std::string& dwStr)
{
    std::string str;
    LogStream ss;
    unsigned int k;
    std::string pbBase64Urlencode;
    ss << GetDownloadUrl();

    BidEncode(ss);

    buildFeedBackField(rtb_resp, pBid, 3, str);
    StringEncode(str, k, pbBase64Urlencode);
    ss << "c=" << k <<"&q=" << pbBase64Urlencode;
    dwStr.assign(ss.buffer().data(), ss.buffer().length());
}

std::string& ProtocolObject::GetWinNoticeUrl()
{
    static std::string def_result = "";
    return def_result;
}

void ProtocolObject::buildFeedBackField(const rtb::BidResponse &rtb_resp,
                                        const rtb::Bid* pBid,
                                        int action_type,
                                        std::string& str)
{
    urlencode->Clear();
    urlencode->set_pid(pBid->impid());
    urlencode->set_dspid(DspID());
    urlencode->set_bid(Id());
    urlencode->set_source(GetSource());
    if (rtb_request_->ext().has_page_pv_id()) {
        urlencode->set_sid(rtb_request_->ext().page_pv_id());
    }
    if (pBid->has_deal_id()) {
        urlencode->set_deal_id(pBid->deal_id());
    }
    urlencode->set_dev_id(rtb_resp.has_dev_id() ? rtb_resp.dev_id() : rtb_request_->device().id());
    if (rtb_request_->user().ext().has_aid()) {
        urlencode->set_aid(rtb_request_->user().ext().aid());
    }
    if (pBid->ext().has_adgroup_id()) {
        urlencode->set_ad_id(pBid->ext().adgroup_id());//广告组id
    }
    if (pBid->has_creative_id()) {
        urlencode->set_creative_id(pBid->creative_id());
    }
    if (pBid->has_inner_advertiser_id()) {
        urlencode->set_advertiser_id(pBid->inner_advertiser_id());
    }
    if (pBid->has_campaign_id()) {
        urlencode->set_campaign_id(pBid->campaign_id());//推广计划
    }
    if (pBid->has_price()) {
        urlencode->set_bid_price(pBid->price());
    }
    if (pBid->ext().has_settle_price()) {
        urlencode->set_pdb_price(pBid->ext().settle_price());//媒体结算价格
    }
    if (pBid->ext().has_cost_price()) {
        urlencode->set_cost_price(pBid->ext().cost_price());//广告主结算价格
    }
    if (pBid->ext().has_billing_type()) {
        urlencode->set_billing_type(pBid->ext().billing_type());
    }
    urlencode->set_action(action_type);
    if (pBid->ext().has_campaign_daily_budget()) {
        urlencode->set_campaign_daily_budget(pBid->ext().campaign_daily_budget());
    }
    if (pBid->ext().has_send_speed()) {
        urlencode->set_smooth_type(pBid->ext().send_speed());
    }
    /*此字段为uint32类型，对于时间是溢出的。故暂不使用
    if (pBid->ext().has_post_hours()) {
        urlencode->set_post_hours(pBid->ext().post_hours());
    }
    */
    //"acookie=";
    if (pBid->ext().has_org_price()) {
        urlencode->set_org_price(pBid->ext().org_price());
    }
    urlencode->set_timestamp(recv_time_ / 1000);
    if (pBid->ext().has_advertiser_budget()) {
        urlencode->set_advertiser_daily_budget(pBid->ext().advertiser_budget());
    }
    if (pBid->has_traffic_bid_flag()) {
        urlencode->set_traffic_bid_flag(pBid->traffic_bid_flag());// 竞价流量标记 0:正常竞价 1:出价探测
    }
    if (pBid->ext().has_freq_impression()) {
        urlencode->set_freq(pBid->ext().freq_impression());
    }
    if (pBid->has_material_id()) {
        urlencode->set_creative_material_id(pBid->material_id());//素材ID
    }
    if (pBid->has_ch()) {
        urlencode->set_ch(pBid->ch());
    }
    urlencode->set_trace_id(rtb_resp.trace_id());
    if (pBid->has_view_type()) {
        urlencode->set_view_type(pBid->view_type());
    }
    if (pBid->has_premium_rate()) {
        urlencode->set_premium_rate(pBid->premium_rate());
    }
    for(int i = 0; i < rtb_resp.algo_feedbacks_size(); ++i) {
        const poseidon::util::KeyValue &kv = rtb_resp.algo_feedbacks(i);
        auto *fb = urlencode->add_algo_feedbacks();
        fb->CopyFrom(kv);
        //前10串
        if (fb->key() == "keywords" && fb->has_value()) {
            char *p = (char*)fb->value().c_str();
            char *e = p;
            int pos = 0;
            while (*p) {
                while (*p && *p != '|') {
                    ++p;
                }
                if (!*p) {
                    break;
                }
                if (++pos == 10) {
                    fb->set_value(std::string(e, p));
                    break;
                }
                ++p;
            }
        }
    }
    std::string *expids = urlencode->mutable_exp_id();
    char exp_id[32];
    for(int i = 0; i < rtb_resp.exp_id_size(); ++i) {
        snprintf(exp_id, sizeof(exp_id), "%d|", rtb_resp.exp_id(i));
        expids->append(exp_id);
    }
    if (pBid->has_gid()) {
        urlencode->set_gid(pBid->gid());
    }

    if (rtb_request_->device().os() == "android") {
        urlencode->set_os(1);
    } else if (rtb_request_->device().os() == "ios") {
        urlencode->set_os(2);
    }
    urlencode->SerializeToString(&str);
}

const char* ProtocolObject::CreateTraceId()
{
    LogStream ss;
    ss << GetSource() << CurrentThread::tid() << sequence_id++ << Id() << recv_time_;
    TcpConnectionPtr conn = wptr_.lock();
    if(conn) {
        const InetAddress& local = conn->localAddress();
        const InetAddress& peer = conn->peerAddress();
        ss << local.ipNetEndian() << local.portNetEndian()
           << peer.ipNetEndian() << peer.portNetEndian();
    }
    //snprintf(trace_id_buff, sizeof(trace_id_buff), "%d-%d-%lld-%s", source, CurrentThread::tid(),
    //                                        sequence_id++, request_object->Id().c_str());
    md5(ss.buffer().data(), trace_id_md5_buff, sizeof(trace_id_md5_buff));
    return trace_id_md5_buff;
}


bool ProtocolObject::StringEncode(const std::string &src, unsigned int &crc16, std::string &encodeStr)
{
    int len = static_cast<int>(src.length());
    //char* enBuf = new char[len];
    char enBuf[1024 * 15];//xxxx:考虑到http url的长度，此处buf限定为15k，足够了。
    if(src.length() > sizeof(enBuf)) {
        LOG_ERROR << "src length is too long!";
        return false;
    }
    for(int i = 0; i < len; ++i) {
        enBuf[i] = src[i] ^ XOR_KEY;
    }
    unsigned int c_k = StringCodec::CRC16(enBuf, len);
    std::string pbBase64;
    StringCodec::Base64Encode(enBuf, len, pbBase64);
    StringCodec::UrlEncode(pbBase64, encodeStr);
    //delete[] enBuf;

    crc16 = (XOR_KEY_VER ^ len ) + (c_k << 16);
    return true;
}

int64_t ProtocolObject::Now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

int64_t ProtocolObject::GetMS()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

std::string ProtocolObject::Compress(const std::string &str)
{
    char buf[1024 * 64];
    uLong blen = sizeof(buf);
    /* 计算缓冲区大小，并为其分配内存 */
    //blen = compressBound(str.length()); /* 压缩后的长度是不会超过blen的 */
    //if((buf = (char*)malloc(sizeof(char) * blen)) == NULL)
    //{
    //    printf("no enough memory!\n");
    //    return -1;
    //}
    /* 压缩 */
    if(compress2((Bytef*)buf, &blen, (Bytef*)str.c_str(), str.length(), 9) != Z_OK) {
        LOG_ERROR << "zlib compress failed!";
        return "";
    }
    return std::move(std::string(buf, blen));
}

size_t ProtocolObject::Compress(const char *src,size_t src_len, char *dst, size_t dst_len)
{
    uLong blen = dst_len;
    /* 计算缓冲区大小，并为其分配内存 */
    //blen = compressBound(str.length()); /* 压缩后的长度是不会超过blen的 */
    //if((buf = (char*)malloc(sizeof(char) * blen)) == NULL)
    //{
    //    printf("no enough memory!\n");
    //    return -1;
    //}
    /* 压缩 */
    if(compress2((Bytef*)dst, &blen, (Bytef*)src, src_len, 9) != Z_OK) {
        LOG_ERROR << "zlib compress failed!";
        return -1;
    }
    return blen;
}

std::string ProtocolObject::UnCompress(const char *src, size_t len)
{
    char buff[1024 * 64];
    uLong tlen = sizeof(buff);
    /* 解压缩 */
    if(uncompress((Bytef*)buff, &tlen, (Bytef*)src, len) != Z_OK) {
        LOG_ERROR << "zlib uncompress failed!";
        return "";
    }
    return std::move(std::string(buff, tlen));
}

size_t ProtocolObject::UnCompress(const char *src, size_t src_len, char *dst, size_t dst_len)
{
    uLong tlen = dst_len;
    /* 解压缩 */
    if(uncompress((Bytef*)dst, &tlen, (Bytef*)src, src_len) != Z_OK) {
        LOG_ERROR << "zlib uncompress failed!";
        return -1;
    }
    return tlen;
}

std::string ProtocolObject::UnCompress(const std::string &str)
{
    char buff[1024 * 64];
    uLong tlen = sizeof(buff);
    /* 解压缩 */
    if(uncompress((Bytef*)buff, &tlen, (Bytef*)str.c_str(), str.length()) != Z_OK) {
        LOG_ERROR << "zlib uncompress failed!";
        return "";
    }
    return std::move(std::string(buff, tlen));
}

void ProtocolObject::AddTimeIds(const std::string &trace_id, const muduo::net::TimerId &time_id)
{
    timeout_ids_.insert(std::make_pair(trace_id, time_id));
}

muduo::net::TimerId ProtocolObject::GetTimeOutId(const std::string &trace_id)
{
    auto iter = timeout_ids_.find(trace_id);
    if(iter != timeout_ids_.end()) {
        return iter->second;
    }
    return muduo::net::TimerId();
}

