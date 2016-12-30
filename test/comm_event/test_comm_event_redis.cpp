/**
 **/


//include STD C head files
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <hiredis.h>

//include STD C++ head files


//include third_party_lib head files
#include "comm_event_redis.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"
#include "poseidon_proto.h"
using namespace dc::common::comm_event;

std::string cmd; 

#define LOG(fmt, a...) printf("[%d in %s]%s:"fmt"\n", __LINE__, __FILE__, __FUNCTION__, ##a)

class MyRedis:public CommRedis
{
public:

    virtual int handle_get_result(void * sess_data, void * pRedisReply)
    {
        LOG("sess_data[%ld]", (long)sess_data);
        ::redisReply * r=(::redisReply *)pRedisReply;
        std::cout<<r->str<<std::endl;
        disconnect();
        return 0;
    }

    /**
     * @brief               链接完成时被调用
     **/
    virtual void on_connected()
    {
        LOG("______");
        long sessid=16;
        send_cmd((void *)sessid, cmd.c_str());
    }

    /**
     * @brief               链接断开时被
     **/
    virtual void on_disconnect()
    {
        LOG("______");
        /*断开重连*/
        reconnect();
        CommFactoryInterface::instance().add_comm_redis(this);
    }

    virtual void on_error(const char * errstr)
    {
        LOG("______");
    }

};

int main(int argc, char * argv [])
{
    int rt=0;
    do{
        if(CommFactoryInterface::instance().init() != EC_SUCCESS)
        {
            printf("CommFactoryInterface::instance().init() return error\n");
            rt=-1;
            break;
        }
        const char * pip="0.0.0.0";
        int port=1234;
        if(argc >=  2)
        {
            pip=argv[1];
        }
        if(argc >= 3)
        {
            port=atoi(argv[2]);
        }if(argc >= 4)
        {
            cmd=argv[3];
        }
        
        CommRedis * redis=new MyRedis();
        redis->connect(pip, port);
        CommFactoryInterface::instance().add_comm_redis(redis);
        CommFactoryInterface::instance().run();
        delete redis;
    }while(0);
    return rt;

}

