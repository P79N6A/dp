#include "ProtocolObjFactory.h"
#include "ProtocolObject.h"
#include <muduo/net/EventLoop.h>

using namespace poseidon;
using namespace poseidon::adapter;

char ProtocolObjFactory::inited_ = 0;

ProtocolObjFactory::ProtocolObjFactory()
{
    MamaPb_pool_destroy = boost::bind(&boost::object_pool<MaMaPBObject>::destroy,
                                      boost::ref(MamaPb_pool), _1);
    YoukuJson_pool_destroy = boost::bind(&boost::object_pool<YoukuJsonObject>::destroy,
                                         boost::ref(YoukuJson_pool), _1);
    Aligame_pool_destroy = boost::bind(&boost::object_pool<AliGameJsonObject>::destroy,
                                       boost::ref(Aligame_pool), _1);
    Chance_pool_destroy = boost::bind(&boost::object_pool<ChanceJsonObject>::destroy,
                                      boost::ref(Chance_pool), _1);
    Yunos_pool_destroy = boost::bind(&boost::object_pool<YunOsPbObject>::destroy,
                                     boost::ref(Yunos_pool), _1);
    Iqiyi_pool_destroy = boost::bind(&boost::object_pool<IQiYiPBObject>::destroy,
                                     boost::ref(Iqiyi_pool), _1);
    Todaytoutiao_pool_destroy = boost::bind(&boost::object_pool<TodayToutiao>::destroy,
                                            boost::ref(Todaytoutiao_pool), _1);
    Wax_pool_destroy = boost::bind(&boost::object_pool<WaxJsonObject>::destroy,
                                   boost::ref(Wax_pool), _1);
    Sohu_pool_destroy = boost::bind(&boost::object_pool<SohuPbObject>::destroy,
                                   boost::ref(Sohu_pool), _1);
}

ProtocolObjFactory::~ProtocolObjFactory()
{
    //dtor
}

//主线程初始化一次
void ProtocolObjFactory::InitStaticOnce(muduo::net::EventLoop *loop)
{
    if(inited_) {
        return;
    }
    ProtocolObject::InitStaticVar();//初始化类静态变量
    MaMaPBObject::InitStaticVar();
    YoukuJsonObject::InitStaticVar(loop);
    AliGameJsonObject::InitStaticVar();
    ChanceJsonObject::InitStaticVar();
    YunOsPbObject::InitStaticVar();
    IQiYiPBObject::InitStaticVar();
    TodayToutiao::InitStaticVar();
    WaxJsonObject::InitStaticVar();
    SohuPbObject::InitStaticVar();
    inited_ = 1;
}

//工作线程初始化时调用
void ProtocolObjFactory::OnWorkThreadInit()
{
    ProtocolObject::OnThreadInitStatic();
    JsonObject::OnThreadInitStatic();
    CommonPb::OnThreadInitStatic();
    MaMaPBObject::OnThreadInitStatic();
    YoukuJsonObject::OnThreadInitStatic();
    ChanceJsonObject::OnThreadInitStatic();
    AliGameJsonObject::OnThreadInitStatic();
    YunOsPbObject::OnThreadInitStatic();
    IQiYiPBObject::OnThreadInitStatic();
    TodayToutiao::OnThreadInitStatic();
    WaxJsonObject::OnThreadInitStatic();
    SohuPbObject::OnThreadInitStatic();
}

ProtocolObjectPtr ProtocolObjFactory::GetInstance(const std::string &url)
{
    ProtocolObjectPtr instance;
    size_t url_len = url.length();

    if(url_len == TANX_REQUEST_URL_LENGTH && url == TANX_REQUEST_URL) {
        instance.reset(MamaPb_pool.construct(), MamaPb_pool_destroy);
    }

    else if(url_len == YOUTU_REQUEST_URL_LENGTH && url == YOUTU_REQUEST_URL) {
        instance.reset(YoukuJson_pool.construct(), YoukuJson_pool_destroy);
    }

    else if(url_len == ALIGAME_REQUEST_URL_LENGTH && url == ALIGAME_REQUEST_URL) {
        instance.reset(Aligame_pool.construct(), Aligame_pool_destroy);
    }

    else if(url_len == CHANCE_REQUEST_URL_LENGTH && url == CHANCE_REQUEST_URL) {
        instance.reset(Chance_pool.construct(), Chance_pool_destroy);
    }

    else if(url_len == YUNOS_REQUEST_URL_LENGTH && url == YUNOS_REQUEST_URL) {
        instance.reset(Yunos_pool.construct(), Yunos_pool_destroy);
    } else if(url_len == IQIYI_REQUEST_URL_LENGTH && url == IQIYI_REQUEST_URL) {
        instance.reset(Iqiyi_pool.construct(), Iqiyi_pool_destroy);
    } else if(url_len == TODAYTOUTIAO_REQUEST_URL_LENGTH && url == TODAYTOUTIAO_REQUEST_URL) {
        instance.reset(Todaytoutiao_pool.construct(), Todaytoutiao_pool_destroy);
    } else if(url_len == WAX_REQUEST_URL_LENGTH && url == WAX_REQUEST_URL) {
        instance.reset(Wax_pool.construct(), Wax_pool_destroy);
    } else if(url_len == SOHU_REQUEST_URL_LENGTH && url == SOHU_REQUEST_URL) {
        instance.reset(Sohu_pool.construct(), Sohu_pool_destroy);
    }


    if(instance) {
        instance->Init();    //某些不适合在构造函数里初始化的，放在此。比如调用虚函数Id()
    }
    return instance;
}
