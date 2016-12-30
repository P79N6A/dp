/**
 **/

#ifndef  _DN_SERVER_H_
#define  _DN_SERVER_H_

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
#include "json/json.h"
#include "data_api/kv_api.h"
using namespace std;
using namespace poseidon::mem_sync;

namespace poseidon
{
namespace dn
{

enum
{
    FLAG_USER=1,
    FLAG_USER_GAME=(1<<1),
    FLAG_DEVICE=(1<<2),
};

struct SessData
{
    DNRequest dnreq;                            //请求包
    DNResponse dnrsp;                           //相应包
    common::Creative creative;					//创意
    std::string creatives;         //创意列表
	struct sockaddr_in addr;                    //地址
    int status;                                 //状态
};

class DnServer:public dc::common::comm_event::CommBase, public boost::serialization::singleton<DnServer>
{
public:
    /**
     * @brief               process req package
     **/
//	DnServer():redis_context_(NULL)
//	{
//	}


	/**
	 * @brief               redis重新连接
	 **/
	int redis_connect();

	/**
	 * @brief           初始化
	 **/
	int init();


    virtual int handle_read(const char * buf, const int len, struct sockaddr_in & client_addr);

    int reply_client(SessData * sess);

    int send_get_Creative(SessData * sess);

    int get_Creative_mem(SessData * sess);

    int on_get_Creative(void * sess, void * reply , int err_code, const char * err_str);

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
    static int s_on_get_Creative(void * data, void * reply, int err_code, const char * err_str);

    int parse_line(const char * buf,common::Creative & creative);
private:
    std::set<SessData *> sess_;
    Json::Reader reader_;
//    redisContext * redis_context_;  //redis上下文
//    std::string redis_host_;        //redis主机
//    int redis_port_;                //redis端口
    KVApi *ka;
    int data_id;
};

}
}




#endif   // ----- #ifndef _dn_SERVER_H_  -----



