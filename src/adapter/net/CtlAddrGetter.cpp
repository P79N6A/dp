#include "CtlAddrGetter.h"
#include "ha.h"
#include "common/common.h"

#include <boost/bind.hpp>
#include <muduo/base/Logging.h>
#include <gflags/gflags.h>

using namespace poseidon;
using namespace poseidon::ha;
using namespace poseidon::adapter;

DECLARE_bool(zk);
DECLARE_bool(test_addr);

CtlAddrGetter::CtlAddrGetter(muduo::net::EventLoop *loop) : main_loop_(loop),
    addr_list_(new std::vector<struct sockaddr_in>()),
    test_addr_list_(new std::vector<struct sockaddr_in>())
{
    //ctor
    if (FLAGS_zk) {
        //srand(time(NULL));
        DoUpdate(HA_CONTROLER_NAME, addr_list_);
        main_loop_->runEvery(Ha::UPDATE_TIME,
                             boost::bind(&CtlAddrGetter::DoUpdate, this, HA_CONTROLER_NAME, boost::ref(addr_list_)));
        /*if (FLAGS_test_addr) {
            DoUpdate(HA_CONTROLER_TEST_NAME, test_addr_list_);
            main_loop_->runEvery(Ha::UPDATE_TIME + 10,//12s
                                 boost::bind(&CtlAddrGetter::DoUpdate, this, HA_CONTROLER_TEST_NAME, boost::ref(test_addr_list_)));
        }
        */
    }
}

//servername必须为copy类型
void CtlAddrGetter::DoUpdate(const std::string servername, AddrListPtr &list)
{
    std::vector<struct sockaddr_in> tmp;
    HA_INSTANCE().get_addrEx(servername, tmp);
    if(tmp.size() == 0) {
        LOG_ERROR << "servername:" << servername <<
                  " Ha return empty ip address, maybe control has downed!";
        return;
    }

    {
        muduo::MutexLockGuard locker(locker_);
        if(!list.unique()) {
            list.reset(new std::vector<struct sockaddr_in>());
        }
        list->swap(tmp);
    }
}

int CtlAddrGetter::GetTestAddr(struct sockaddr_in & addr)
{
    AddrListPtr testaddrlist;
    {
        muduo::MutexLockGuard locker(locker_);
        testaddrlist = test_addr_list_;
    }

    if(testaddrlist->size() == 0) {
        return -1;
    }
    int64_t v = idx_indicate_test_.getAndAdd(1);
    if(v < 0) {
        idx_indicate_test_.getAndSet(0);
    }
    int idx =  v % testaddrlist->size();
    addr = (*testaddrlist)[idx];
    return 0;
}

int CtlAddrGetter::GetAddr(struct sockaddr_in & addr)
{
    //REPEAT:
    {
        AddrListPtr addrlist;
        {
            muduo::MutexLockGuard locker(locker_);
            addrlist = addr_list_;
        }

        if(addrlist->size() == 0) {
            return -1;
        }
        int64_t v = idx_indicate_.getAndAdd(1);
        if(v < 0) {
            idx_indicate_.getAndSet(0);
        }
        int idx = v % addrlist->size();
        addr = (*addrlist)[idx];
        return 0;
    }
}
