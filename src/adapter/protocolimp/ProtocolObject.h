#ifndef PROTOCOLOBJECT_H
#define PROTOCOLOBJECT_H

#include <string>
#include <sstream>
#include <queue>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/pool/object_pool.hpp>

#include <muduo/base/Logging.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/LogStream.h>
#include <muduo/net/EventLoop.h>

#include "monitor_api.h"
#include "common/common.h"
#include "net/HttpRequest.h"
#include "conf/Configer.h"
#include "utility/StringSplitTools.h"
#include "utility/StringCodec.h"
/*
auth:zhangxianghui
date:2016/06
*/

namespace poseidon
{
namespace adapter
{
#define DECLARE_THREAD_VAR(T, name) __thread T *name = NULL
#define DECLARE_THREAD_VAR_QUEUE(T, name) __thread std::queue<T*> *name##_queue = NULL
#define DECLARE_THREAD_VAR_POOL(T, name) __thread boost::object_pool<T> *name##_pool = NULL
#define DECLARE_THREAD_VAR_LIST(T, name) DECLARE_THREAD_VAR_QUEUE(T, name);DECLARE_THREAD_VAR_POOL(T, name)

#define THREAD_VAR_QUEUE(name) name##_queue
#define THREAD_VAR_POOL(name) name##_pool
#define THREAD_VAR_LIST(name) THREAD_VAR_QUEUE(name), THREAD_VAR_POOL(name)
typedef boost::shared_ptr<rtb::BidRequest> RtbReqSharedPtr;

class ProtocolObject : boost::noncopyable
{
public:

    ProtocolObject();
    virtual ~ProtocolObject();
    void Init();//对象创建后，必须调用Init初始化！

    //从网络接收到数据时(pb,json格式)，由相应的实现来进行解析，若解析成功，则返回true
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t) = 0;
    //解析协议成功后，由相应实现来进行内部协议适配转换。
    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&) = 0;
    //当adxrequest解析请求或rtb的应答协议错误时，子类需要处理的失败情况
    virtual void OnFailed() = 0;
    //RTB应答时，需要实现的内部转换
    virtual int RtbResponse(rtb::BidResponse&, muduo::net::EventLoop*) = 0;

    virtual const char* Id() = 0;//返回每个adx请求的唯一id
    virtual double ResopnseTimeOut() = 0;//获取当前object的应答超时时间

    void SetWeakPtr(TcpConnectionWptr wptr) { wptr_ = wptr; }
    static int64_t Now();//获取当前时间timestamp
    virtual rtb::TrafficSource GetSource() = 0;
    //buffer limits 8kb
    static std::string Compress(const std::string &str);
    static size_t Compress(const char *src,size_t src_len, char *dst, size_t dst_len);
    static std::string UnCompress(const char *str, size_t len);
    static size_t UnCompress(const char *str, size_t src_len, char *dst, size_t dst_len);
    static std::string UnCompress(const std::string &str);
    //每个线程初始化时均会调用此静态方法
    static void OnThreadInitStatic();
    static void InitStaticVar();

    void AddTimeIds(const std::string &trace_id, const muduo::net::TimerId &time_id);
    muduo::net::TimerId GetTimeOutId(const std::string &trace_id);
protected:
    static boost::function<void(rtb::BidRequest *)> dx_;
    //考虑此对象构造析构频繁，故采用静态线程变量来绑定来避免构造和析构的调用
    //每次对象池用完析构时，只对此引用进行Clear;
    RtbReqSharedPtr rtb_request_;
    TcpConnectionWptr wptr_;//TCP连接的弱引用
    int64_t recv_time_;//记录当前对象的创建时间点

    virtual std::string DspID() = 0;
    virtual void buildFeedBackField(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid,
                                    int action_type, std::string& ret_str);
    virtual std::string &GetWinNoticeUrl();//获取竞价胜出地址
    virtual std::string& GetExposeUrl() = 0;//获取曝光地址
    virtual std::string& GetClickUrl() = 0;//获取反馈点击地址
    virtual std::string &GetDownloadUrl() = 0;//获取下载完成地址

    //构建竞价胜出地址 no imp!
    void BuildWinNotice(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid, const std::string &params, std::string &urlStr);
    //构建点击地址
    void BuildClick(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid, const std::string &params, std::string &urlStr);
    //构建曝光地址
    void BuildExpose(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid, const std::string &params, std::string& snippet);
    //构建下载完成地址
    void BuildDownloadcomplete(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid, std::string& dwStr);
    //自定义构建URL，全部参数自定义。适用灵活度比较大的场合
    void CustomBuildUrl(const rtb::BidResponse &rtb_resp,
                                    const rtb::Bid* pBid,
                                    const std::string &host_url,
                                    const std::string &params,
                                    int action_type,
                                    std::string &out);
    bool StringEncode(const std::string &src, unsigned int &crc16, std::string &encodeStr);
    //构建一个唯一ID
    const char* CreateTraceId();
    //rtb::bidrequest分配函数
    static RtbReqSharedPtr Alloc_rtb_req_shared();
    static rtb::BidRequest *Alloc_rtb_req();
    static void Destroy_rtb_req(rtb::BidRequest*);

    //以下是pool&&queue函数
    template<typename T>
    static T* Alloc_Bid_Obj(std::queue<T*> *queue, boost::object_pool<T> *pool)
    {
        T *result;
        if (queue->size() > 0) {
            result = queue->front();
            queue->pop();
        } else {
            result = pool->construct();
        }
        result->Clear();
        return result;
    }
    template<typename T>
    static void InitThreadedQueuePool(std::queue<T*> *&queue, boost::object_pool<T> *&pool)
    {
        if (pool == NULL) {
            pool = new boost::object_pool<T>();//not need release
        }
        if (queue == NULL) {
            queue = new std::queue<T*>();
        }
    }

    template<typename T>
    static void DestroyObject(T *obj, std::queue<T*> *queue, boost::object_pool<T> *pool)
    {
        if (queue->size() < PROTOCOL_OBJECT_QUEUE_MAX_SIZE) {//max
            queue->push(obj);
        } else {
            pool->destroy(obj);
        }
    }
private:
    void BidEncode(muduo::LogStream &ss);
    //trace_id - TimerId
    //把每次请求的traceid和timerid保存起来
    std::map<std::string, muduo::net::TimerId> timeout_ids_;
    int64_t GetMS();
};

}
}


#endif // PROTOCOLOBJECT_H
