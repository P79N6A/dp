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
    void Init();//���󴴽��󣬱������Init��ʼ����

    //��������յ�����ʱ(pb,json��ʽ)������Ӧ��ʵ�������н������������ɹ����򷵻�true
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t) = 0;
    //����Э��ɹ�������Ӧʵ���������ڲ�Э������ת����
    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&) = 0;
    //��adxrequest���������rtb��Ӧ��Э�����ʱ��������Ҫ�����ʧ�����
    virtual void OnFailed() = 0;
    //RTBӦ��ʱ����Ҫʵ�ֵ��ڲ�ת��
    virtual int RtbResponse(rtb::BidResponse&, muduo::net::EventLoop*) = 0;

    virtual const char* Id() = 0;//����ÿ��adx�����Ψһid
    virtual double ResopnseTimeOut() = 0;//��ȡ��ǰobject��Ӧ��ʱʱ��

    void SetWeakPtr(TcpConnectionWptr wptr) { wptr_ = wptr; }
    static int64_t Now();//��ȡ��ǰʱ��timestamp
    virtual rtb::TrafficSource GetSource() = 0;
    //buffer limits 8kb
    static std::string Compress(const std::string &str);
    static size_t Compress(const char *src,size_t src_len, char *dst, size_t dst_len);
    static std::string UnCompress(const char *str, size_t len);
    static size_t UnCompress(const char *str, size_t src_len, char *dst, size_t dst_len);
    static std::string UnCompress(const std::string &str);
    //ÿ���̳߳�ʼ��ʱ������ô˾�̬����
    static void OnThreadInitStatic();
    static void InitStaticVar();

    void AddTimeIds(const std::string &trace_id, const muduo::net::TimerId &time_id);
    muduo::net::TimerId GetTimeOutId(const std::string &trace_id);
protected:
    static boost::function<void(rtb::BidRequest *)> dx_;
    //���Ǵ˶���������Ƶ�����ʲ��þ�̬�̱߳������������⹹��������ĵ���
    //ÿ�ζ������������ʱ��ֻ�Դ����ý���Clear;
    RtbReqSharedPtr rtb_request_;
    TcpConnectionWptr wptr_;//TCP���ӵ�������
    int64_t recv_time_;//��¼��ǰ����Ĵ���ʱ���

    virtual std::string DspID() = 0;
    virtual void buildFeedBackField(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid,
                                    int action_type, std::string& ret_str);
    virtual std::string &GetWinNoticeUrl();//��ȡ����ʤ����ַ
    virtual std::string& GetExposeUrl() = 0;//��ȡ�ع��ַ
    virtual std::string& GetClickUrl() = 0;//��ȡ���������ַ
    virtual std::string &GetDownloadUrl() = 0;//��ȡ������ɵ�ַ

    //��������ʤ����ַ no imp!
    void BuildWinNotice(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid, const std::string &params, std::string &urlStr);
    //���������ַ
    void BuildClick(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid, const std::string &params, std::string &urlStr);
    //�����ع��ַ
    void BuildExpose(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid, const std::string &params, std::string& snippet);
    //����������ɵ�ַ
    void BuildDownloadcomplete(const rtb::BidResponse &rtb_resp, const rtb::Bid* pBid, std::string& dwStr);
    //�Զ��幹��URL��ȫ�������Զ��塣�������ȱȽϴ�ĳ���
    void CustomBuildUrl(const rtb::BidResponse &rtb_resp,
                                    const rtb::Bid* pBid,
                                    const std::string &host_url,
                                    const std::string &params,
                                    int action_type,
                                    std::string &out);
    bool StringEncode(const std::string &src, unsigned int &crc16, std::string &encodeStr);
    //����һ��ΨһID
    const char* CreateTraceId();
    //rtb::bidrequest���亯��
    static RtbReqSharedPtr Alloc_rtb_req_shared();
    static rtb::BidRequest *Alloc_rtb_req();
    static void Destroy_rtb_req(rtb::BidRequest*);

    //������pool&&queue����
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
    //��ÿ�������traceid��timerid��������
    std::map<std::string, muduo::net::TimerId> timeout_ids_;
    int64_t GetMS();
};

}
}


#endif // PROTOCOLOBJECT_H
