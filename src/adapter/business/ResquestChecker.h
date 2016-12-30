#ifndef RESQUESTCHECKER_H
#define RESQUESTCHECKER_H

#include <string>
#include <vector>
#include <set>
#include <map>
//#include <tr1/unordered_map>

#include <google/protobuf/message.h>

#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoopThread.h>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "poseidon_tanx.pb.h"
#include "poseidon_rtb.pb.h"
#include "SqlConnection.h"

namespace muduo
{
namespace net
{
class EventLoopThread ;
}
}


namespace poseidon
{

namespace adapter
{

class MiniRequest;
class ResquestChecker : boost::noncopyable
{
public:
    ResquestChecker();
    virtual ~ResquestChecker();
    int CheckRequest(const rtb::BidRequest&);
    muduo::net::EventLoop* GetLoop() { return loop_; }
private:
    muduo::MutexLock locker_;
    //定时刷新数据库。为避免影响IO线程，单独使用定时线程。
    muduo::net::EventLoopThread loop_thread_;
    //key:column_name,   value:set<column_value>!!
    typedef std::map<std::string, std::set<std::string> > BlackWiteCotinerType;
    typedef std::set<std::string> SetElementType;
    typedef boost::shared_ptr<BlackWiteCotinerType> BlackWiteCotinerType_ptr;
    BlackWiteCotinerType_ptr black_wite_sets_;

    //key:column_names, value:vector<pair(column_name, column_value)>
    typedef std::map<std::string, std::vector<std::pair<std::string, std::string> > > CheatContinerType;
    typedef std::vector<std::pair<std::string, std::string> > VectorElementType;
    typedef boost::shared_ptr<CheatContinerType> CheatContinerType_ptr;
    CheatContinerType_ptr cheat_records_;

    const google::protobuf::DescriptorPool *desc_pool_;
    google::protobuf::MessageFactory *message_factory_;
    muduo::net::EventLoop *loop_;
    //
    google::protobuf::Message* GetMessageByTypeName(const std::string&);
    int CheckBlackWite(const rtb::BidRequest&);
    int CheckCheat(const rtb::BidRequest&);
    //定时刷新数据库数据
    void RefreshDbData();

};

}
}

#endif // RESQUESTCHECKER_H
