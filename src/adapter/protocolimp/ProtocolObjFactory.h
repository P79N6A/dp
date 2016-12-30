#ifndef PROTOCOLOBJFACTORY_H
#define PROTOCOLOBJFACTORY_H

//#include <unordered_map>//erase有性能瓶颈
#include <boost/unordered_map.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "YoukuJsonObject.h"
#include "MaMaPBObject.h"
#include "AliGameJsonObject.h"
#include "ChanceJsonObject.h"
#include "YunOsPbObject.h"
#include "IQiYiPBObject.h"
#include "TodayToutiao.h"
#include "WaxJsonObject.h"
#include "SohuPbObject.h"

/*
auth: zhangxh
date: 2016-09
*/

namespace poseidon
{

namespace adapter
{


typedef boost::shared_ptr<ProtocolObject> ProtocolObjectPtr;
typedef boost::unordered_map<std::string, ProtocolObjectPtr> RequestMapCache;
typedef boost::unordered_map<std::string, ProtocolObjectPtr>::iterator RequestMapCache_Iterator;


class ProtocolObjFactory : boost::noncopyable
{
public:
    ProtocolObjFactory();
    ~ProtocolObjFactory();
    ProtocolObjectPtr GetInstance(const std::string &url);
    //主线程初始化一次
    static void InitStaticOnce(muduo::net::EventLoop *loop);
    static void OnWorkThreadInit();
private:
    static char inited_;
    boost::function<void(MaMaPBObject*)> MamaPb_pool_destroy;
    boost::object_pool<MaMaPBObject> MamaPb_pool;
    boost::function<void(YoukuJsonObject*)> YoukuJson_pool_destroy;
    boost::object_pool<YoukuJsonObject> YoukuJson_pool;
    boost::function<void(AliGameJsonObject*)> Aligame_pool_destroy;
    boost::object_pool<AliGameJsonObject> Aligame_pool;
    boost::function<void(ChanceJsonObject*)> Chance_pool_destroy;
    boost::object_pool<ChanceJsonObject> Chance_pool;
    boost::function<void(YunOsPbObject*)> Yunos_pool_destroy;
    boost::object_pool<YunOsPbObject> Yunos_pool;
    boost::function<void(IQiYiPBObject*)> Iqiyi_pool_destroy;
    boost::object_pool<IQiYiPBObject> Iqiyi_pool;
    boost::function<void(TodayToutiao*)> Todaytoutiao_pool_destroy;
    boost::object_pool<TodayToutiao> Todaytoutiao_pool;
    boost::function<void(WaxJsonObject*)> Wax_pool_destroy;
    boost::object_pool<WaxJsonObject> Wax_pool;
    boost::function<void(SohuPbObject*)> Sohu_pool_destroy;
    boost::object_pool<SohuPbObject> Sohu_pool;

};

}

}

#endif // PROTOCOLOBJFACTORY_H
