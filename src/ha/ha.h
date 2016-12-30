/**
 **/

#include "../zk4cpp/zk4cpp.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <vector>
#include <boost/serialization/singleton.hpp>
#include <boost/thread/mutex.hpp>

namespace poseidon
{
namespace ha
{
class Ha:public Zk4Cpp, public boost::serialization::singleton<Ha>
{
public:

    enum
    {
        UPDATE_TIME=2,      //2秒钟更新一次
        DEF_QUOTA=5,        //默认配额
        MAX_QUOTA=20,       //最大的配额
    };


    /**
     * @brief               初始化
     * @param zk_iplist     [IN], zk iplist
     * @return              success return 0, or return other
     **/
    int init(const std::string zk_iplist);


    /**
     * @brief               注册服务
     * @param servername    [IN], 服务名
     * @param ip            [IN], IP
     * @param port          [IN], 端口
     * @return              success return 0, or return other
     **/
    int reg_server(const std::string & servername, const std::string & ip, int port, int quota=DEF_QUOTA);


    /**
     * @brief               根据servername获取一个地址
     * @param servername    [IN],服务名
     * @param addr          [OUT], 返回地址
     * @return              success return 0, or return other
     **/
    int get_addr(const std::string & servername, struct sockaddr_in & addr  );

#if 0
    //顺序获取目标地址，区别于随机获取
    int get_addr_seq(const std::string & servername, struct sockaddr_in & addr  );
#endif
    //xxxx add:此函数不要频繁调用，间隔UPDATE_TIME即可
    void get_addrEx(const std::string &servername, std::vector<struct sockaddr_in> &vec);

    bool wait_connect_done();

    virtual void onExpired();
    virtual void onConnect();


private:

    int update_serverinfo(const std::string & servername);

    typedef struct {
        std::string server_name_;
        int update_time_;
        int serial_num_;
        std::vector<struct sockaddr_in> addr_list_;
    }ServerInfo;

    std::map<std::string, ServerInfo >  map_serverinfo_;

    typedef struct
    {
        std::string ip;
        int port;
        int quota;
    }RegInfo;

    std::map<std::string, RegInfo> map_reg_info_ ;
    boost::mutex lock_;
//    uint64_t serial_num_;

};

}
}

#define HA_REG(server, ip, port) poseidon::ha::Ha::get_mutable_instance().reg_server(server, ip, port)

#define HA_REG_Q(server, ip, port, quota) poseidon::ha::Ha::get_mutable_instance().reg_server(server, ip, port, quota)

#define HA_GET_ADDR(server, addr) poseidon::ha::Ha::get_mutable_instance().get_addr(server, addr)

#define HA_INIT(iplist) poseidon::ha::Ha::get_mutable_instance().init(iplist)

#define HA_INSTANCE() poseidon::ha::Ha::get_mutable_instance()


