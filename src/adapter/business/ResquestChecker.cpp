#include "ResquestChecker.h"
#include "net/UdpChannel.h"
#include "conf/Configer.h"
#include "utility/StringSplitTools.h"

#include <vector>
#include <algorithm>

#include <muduo/base/Logging.h>

#include <boost/bind.hpp>


using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace poseidon;
using namespace poseidon::adapter;
using namespace google::protobuf;

extern Configer * volatile configer;

ResquestChecker::ResquestChecker() :
    loop_thread_(NULL, "MiscLoopThead"),
    black_wite_sets_(new BlackWiteCotinerType()),
    cheat_records_(new CheatContinerType()),
    desc_pool_(DescriptorPool::generated_pool()),
    message_factory_(MessageFactory::generated_factory()),
    loop_(loop_thread_.startLoop())
{

    RefreshDbData();
    //ctor
    int refresh_internal = configer->refresh_internal()/*分钟*/* 60;
    //定时从DB中获取刷新数据
    loop_->runEvery(refresh_internal, bind(&ResquestChecker::RefreshDbData, this));
}

ResquestChecker::~ResquestChecker()
{
    //dtor
}

//定时刷新数据
void ResquestChecker::RefreshDbData()
{
    SqlConnection con;
    if(!con.Connect()) {
        LOG_ERROR << "failed to connect DB!";
        return;
    }

    vector<EntieBlackWite> vec_black;
    vector<EntieAgainstCheat> vec_cheat;
    vector<EntieBlackWite>::iterator iter_black;
    vector<EntieAgainstCheat>::iterator iter_cheat;
    BlackWiteCotinerType black_wite_temp;
    CheatContinerType cheat_temp;

    LOG_INFO << "Refresh DB Data Begin...";
    //Black record:  field-value
    con.GetBlackWite(vec_black);
    //cheat record: field,field,field-value,value,value
    con.GetAgainstCheat(vec_cheat);
    LOG_INFO << "Refresh DB Data End...";

    iter_black = vec_black.begin();
    for(; iter_black != vec_black.end(); ++iter_black) {
        black_wite_temp[iter_black->field_name].insert(iter_black->field_value);
    }

    iter_cheat = vec_cheat.begin();//db records
    vector<std::string> vec_field;
    vector<std::string> vec_value;
    for(; iter_cheat != vec_cheat.end(); ++iter_cheat) {
        vec_field.clear(), vec_value.clear();
        std::string key;
        StringSplitTools::splitString(iter_cheat->field_names, ',', vec_field);
        StringSplitTools::splitString(iter_cheat->expression, ',', vec_value);
        //copy(vec_field.begin(), vec_field.end(), back_inserter(ordedvector));
        //把fieldname重新排序，作为key，这样相同字段但不同位置的记录可以同样合并在一个vector中
        //sort(ordedvector.begin(), ordedvector.end()); //xxxx: 考虑下，还是不要合并了。不同记录是or关系不是and关系
        for(size_t i = 0; i < vec_field.size(); ++i) {
            key += vec_field[i];
        }
        //防止字段数量与实际值数量不等的情况，同时也保证了空字段是无法形成一条有效作弊记录
        size_t min_size = vec_field.size() > vec_value.size() ? vec_value.size() : vec_field.size();
        for(size_t i = 0; i < min_size; ++i) {
            cheat_temp[key].push_back(make_pair(vec_field[i], vec_value[i]));
        }
    }

    {
        MutexLockGuard locker(locker_);
        if(!black_wite_sets_.unique()) { //若有读线程正在使用，则重新分配一块数据区
            black_wite_sets_.reset(new BlackWiteCotinerType);
        }
        black_wite_sets_->swap(black_wite_temp);
    }

    {
        MutexLockGuard locker(locker_);
        if(!cheat_records_.unique()) {
            cheat_records_.reset(new CheatContinerType);
        }
        cheat_records_->swap(cheat_temp);
    }
}

int ResquestChecker::CheckCheat(const rtb::BidRequest &rtb_req)
{

    CheatContinerType_ptr cheat_records_local;
    {
        MutexLockGuard locker(locker_);
        cheat_records_local = cheat_records_; //add ref count
    }


    CheatContinerType::iterator iter = cheat_records_local->begin();
    VectorElementType::iterator values_iter;
    if(cheat_records_local->size() == 0) {
        return 0;
    }

    bool ischeat = true;//默认是反作弊名单
    //需要遍历反作弊所有记录
    for(; iter != cheat_records_local->end(); ++iter) {
        if(iter->second.size() == 0) {
            continue;    //xxxx:impossable!
        }
        values_iter = iter->second.begin();
        for(; values_iter != iter->second.end(); ++values_iter) {
            if(values_iter->first == "dev_id" && !rtb_req.device().id().empty()) {//对应rtb::Device::id
                ischeat &= (values_iter->second == rtb_req.device().id());
            } else if(values_iter->first == "acookie" && !rtb_req.user().ext().acookie().empty()) {
                ischeat &= (values_iter->second == rtb_req.user().ext().acookie());
            }
            //sid = page_session_id = page_pv_id
            else if(values_iter->first == "sid" && !rtb_req.ext().page_pv_id().empty()) {
                ischeat &= (values_iter->second == rtb_req.ext().page_pv_id());
            } else if(values_iter->first == "ip" && !rtb_req.device().ip().empty()) {
                ischeat &= (values_iter->second == rtb_req.device().ip());
            } else {
                return 0;    //存在非法字段，那么默认返回非作弊标志
            }
        }
        //若一条记录遍历完ischeat还是为true的话，说明符合某一条规则，那么直接返回-1。即属于反作弊名单条件
        if(ischeat) {
            return -1;
        }
    }
    return 0;
}

//若属于黑名单，返回非0
int ResquestChecker::CheckBlackWite(const rtb::BidRequest &rtb_req)
{
    BlackWiteCotinerType_ptr black_wite_sets_local;
    {
        MutexLockGuard locker(locker_);
        black_wite_sets_local = black_wite_sets_;//add ref count
    }

    BlackWiteCotinerType::iterator iter = black_wite_sets_local->begin();
    SetElementType::iterator set_iter;
    //imei,idfa,mac,aid,acookie,dspid,sid,ip = key
    for(; iter != black_wite_sets_local->end(); ++iter) {//最多8条记录
        if(iter->first == "dev_id" && !rtb_req.device().id().empty()) {//对应rtb::Device::id
            set_iter = iter->second.find(rtb_req.device().id());//能找到记录说明在黑名单里头
            if(set_iter != iter->second.end()) {
                return -1;
            }
        }
        //request.tid
        else if(iter->first == "acookie" && !rtb_req.user().ext().acookie().empty()) {
            set_iter = iter->second.find(rtb_req.user().ext().acookie());
            if(set_iter != iter->second.end()) {
                return -1;
            }
        }
        //sid = page_session_id = page_pv_id
        else if(iter->first == "sid" && !rtb_req.ext().page_pv_id().empty()) {
            set_iter = iter->second.find(rtb_req.ext().page_pv_id());
            if(set_iter != iter->second.end()) {
                return -1;
            }
        } else if(iter->first == "ip" && !rtb_req.device().ip().empty()) {
            set_iter = iter->second.find(rtb_req.device().ip());
            if(set_iter != iter->second.end()) {
                return -1;
            }
        } else { //非8个预定字段的值，那么返回非黑名单标志
            return 0;
        }
    }
    return 0;
}

//若属于黑名单或反作弊名单，返回非0
int ResquestChecker::CheckRequest(const rtb::BidRequest &rtb_req)
{
    if(CheckBlackWite(rtb_req) != 0) {
        LOG_INFO <<"id:" << rtb_req.id() << " is Black user!";
        return -1;
    }
    if(CheckCheat(rtb_req) != 0) {
        LOG_INFO <<"id:" << rtb_req.id() << " is Cheat user!";
        return -1;
    }
    return 0;
}


//type_name: eg:tutorial.Person.PhoneNumber
Message* ResquestChecker::GetMessageByTypeName(const std::string &type_name)
{
    const Descriptor *desc = desc_pool_->FindMessageTypeByName(type_name);
    if(desc) {
        const Message *proto_type = message_factory_->GetPrototype(desc);
        if(proto_type) {
            return proto_type->New();
        }
    }
    return NULL;
}
