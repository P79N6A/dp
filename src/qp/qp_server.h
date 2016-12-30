/**
 **/

#ifndef  _QP_SERVER_H_ 
#define  _QP_SERVER_H_

#include <boost/serialization/singleton.hpp>
#include "comm_event.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "protocol/src/poseidon_proto.h"
#include "redis_access.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>
#include <map>


namespace poseidon
{
namespace qp
{

enum
{
    FLAG_USER=1,
    FLAG_USER_GAME=(1<<1),
    FLAG_DEVICE=(1<<2),
};

struct SessData
{
    QPRequest qpreq;                            //请求包
    QPResponse qprsp;                           //相应包
    std::string imei;                           //imei号
    struct sockaddr_in addr;                    //地址
    std::map<int, std::string> tags;            //用户标签
    int status;                                 //状态
};

enum TagId
{
    TAGID_GRENDER=1001,
    TAGID_AGE=1002,
    TAGID_IDCARD_AGE=1008,
    TAGID_DEGREE=1003,
    TAGID_CAREER=1004,
    TAGID_COLLEGE_STUDENT=1005,
    TAGID_MARRIAGE=1006,
    TAGID_LOCATION=1007,
    TAGID_DEVICE_PRICE=2001,
    TAGID_OS=2002,
    TAGID_BRAND=2005,
    TAGID_MODEL=2006,
    TAGID_CARRIER=2003,
    TAGID_NETWORK=2004,
    TAGID_CONTENT_CATEGORIES=3001,
    TAGID_APP_CATEGORIES=3002,
    TAGID_CATEGORY=3003,
    TAGID_PAY_USER=4001,
    TAGID_GAME_CATEGORIES=4002,
    TAGID_GAME_THEME=4003,
    TAGID_GAME_PLAY=4004,
    TAGID_GAME_CULTURE=4005,
    TAGID_GAME_FEATURE=4006,
    TAGID_GAME_LEVEL=4007,
    TAGID_LAST_LOGIN_GAME=4008,
    TAGID_LAST_PAY_GAME=4009,
    TAGID_FIRST_PAY_GAME=4010,
    TAGID_USER_LEVEL=5001,
};


class QpServer:public dc::common::comm_event::CommBase, public boost::serialization::singleton<QpServer>
{
public:
    /**
     * @brief               process req package
     **/
    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);

    int reply_client(SessData * sess);


    int send_get_user_tag(SessData * sess);
    int send_get_user_game_tag(SessData * sess);
    int send_get_device_price(SessData * sess);

    int on_get_user_tag(void * sess, void * reply , int err_code, const char * err_str);
    int on_get_user_game_tag(void * sess, void * reply, int err_code, const char * err_str );
    int on_get_device_price(void * sess, void * reply, int err_code, const char * err_str);

    int proc_sess(SessData * sess);

    SessData * alloc_sess()
    {
        SessData * p=NULL;
        p=new(std::nothrow) SessData();
        if(p != NULL)
        {
            sess_.insert(p);
            p->status=0;
            return p;
        }else
        {
            return NULL;
        }
    }
    void free_sess(SessData * sess)
    {
        if(sess_.count(sess) > 0)
        {
            delete sess;
            sess_.erase(sess);
        }
    }
    bool sess_valid(SessData * sess)
    {
        return sess_.count(sess)>0;
    }
    static int s_on_get_user_tag(void * data, void * reply, int err_code, const char * err_str);
    static int s_on_get_user_game_tag(void * data, void * reply , int err_code, const char * err_str);
    static int s_on_get_device_price(void * data, void * reply , int err_code, const char * err_str);

private:
    std::set<SessData *> sess_;

};

}
}


#endif   // ----- #ifndef _QP_SERVER_H_  ----- 



