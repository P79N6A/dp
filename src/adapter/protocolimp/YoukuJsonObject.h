#ifndef YOUTU_JSONOBJECT_H
#define YOUTU_JSONOBJECT_H

#include "JsonObject.h"

/*
auth:xxxx
date:2016-07
*/

namespace poseidon
{
namespace adapter
{

//����jsoncppת������asIntʱ�����������ͷ�isInt()����assert����
//��˴˺��Ȱ��ֶ�ת�ɰ�ȫ��String���ͣ���ת����Int����
//#define ASINT(OBJ) (atoi(OBJ.asCString()))

class YoukuJsonObject : public JsonObject
{
public:
    YoukuJsonObject();
    virtual ~YoukuJsonObject();

    virtual int OnRequest(const HttpRequest*, std::vector<RtbReqSharedPtr>&);
    virtual void OnFailed();
    virtual int RtbResponse(poseidon::rtb::BidResponse &rtb_response, muduo::net::EventLoop*);
    virtual bool ParseFromBuff(const HttpRequest*, const char*, size_t);
    virtual double ResopnseTimeOut() { return youtu_resp_timeout_; }
    static void InitStaticVar(muduo::net::EventLoop*);
    static void OnThreadInitStatic();
protected:
    virtual std::string& GetClickUrl() { return click_url_; }
    virtual std::string& GetExposeUrl() { return expose_url_; }
    virtual std::string& GetDownloadUrl() { return downloaded_url_; }
    virtual rtb::TrafficSource GetSource() { return source_; }
    virtual std::string DspID() { return dspid_; }
    virtual const char* Id() { return root_["id"].asCString(); }
private:
    //�ſ���λ��Ϣ
    class Youtuadplacements
    {
     public:
        int adplacementid_;//���λid
        std::string adplacementname_;//����
        std::string size_;//��С 800*400
        int bidfloor_;//���λ�׼ۣ���λ�ǣ���/CPM
        std::set<std::string> blockcategory_;//�������ҵ�������������ֵ�Ǻ�������ҵ����ҵID
        std::set<std::string> allowmaterial_;//������ز����ͣ������Ǻ�׺����ע�⣬�ſ���Ƶ��Ƭ�ز�����������flv�����
    };
    static std::string dspid_;
    static std::string click_url_;
    static std::string expose_url_;
    static std::string downloaded_url_;
    static rtb::TrafficSource source_;
    static double youtu_resp_timeout_;
    static std::map<std::string, int> pid_viewtype_map_;
    //���λid-class Youtuadplacements
    static std::map<int, Youtuadplacements> pid_object_map_;//��ʱδʹ��

    static void ViewMap();
    static void MapPid(const std::string &pids, int view_type);
    static void GetAdPlacements(muduo::net::EventLoop*);
    ////////////////////////////////////////////////
    std::map<std::string, const OpenDspJson::Value *> traceid_impression_;
    int SetImp(const OpenDspJson::Value &imp);
    void SetImp_Banner(const OpenDspJson::Value &imp, const OpenDspJson::Value &banner, rtb::Impression *pImp);
    void SetImp_Video(const OpenDspJson::Value &imp,  const OpenDspJson::Value &video, rtb::Impression *pImp);
    int SetImp_Native(const OpenDspJson::Value &imp,  const OpenDspJson::Value &native, rtb::Impression *pImp);
    void SetImp_Pmp(const OpenDspJson::Value &pmp, rtb::Impression *pImp);
    void SetImp_Ext(const OpenDspJson::Value &ext, rtb::Impression_Ext* pImExt);

    int SetSite(const OpenDspJson::Value &site);
    void SetSiteApp_Content(const OpenDspJson::Value &content, rtb::Content *pContent);
    int SetApp(const OpenDspJson::Value &app);
    int SetDevice(const OpenDspJson::Value &device);
    int SetUser(const OpenDspJson::Value &user);

/////////////////////////////////////////////////
    void SetResponse(const rtb::BidResponse &,const rtb::Bid &rtb_bid, OpenDspJson::Value &bid);

/////////////////////////////////////////////////
    void YouKuSuccResponse(TcpConnectionWptr &wptr, const std::string &response);
    void YouKuFailedResponse(TcpConnectionWptr &wptr);
};


}
}
#endif // YOUTU_JSONOBJECT_H
