#ifndef CONFIGER_H
#define CONFIGER_H

/**
    auth: xxxx
    date: 2016/05
**/

#include <string>
#include <vector>
#include <set>

#include <boost/shared_ptr.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/noncopyable.hpp>

#include <openssl/blowfish.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>


namespace poseidon
{

namespace adapter
{

class UdpChannel;

class Configer : boost::noncopyable
{
public:
    explicit Configer(const std::string &);
    virtual ~Configer();

    std::string dspid();
    int maxfd();
    int threads();
    short int port();
    std::string loglevel();
    static muduo::Logger::LogLevel translatelog(const char *);
    double requesttimeout();
    void UdpRead(UdpChannel*, muduo::net::EventLoop*,
                 muduo::net::InetAddress&, const char*, int);
    //////sql////////
    std::string sql_host();
    std::string sql_user();
    std::string sql_pass();
    int sql_port();
    std::string sql_db();
    int refresh_internal();

    //////sql end////////

    const BF_KEY *Get_Bf_Key() { return &bf_key; }
    int Get_Bf_Key_Ver();

    template <typename T>
    T GetProperty(const char* cof_path, T defvalue) {
        return cfgtree_.get<T>(cof_path, defvalue);
    }

    std::string GetString(const std::string &path);

    int GetInt(const std::string &path);

    typedef std::vector<std::pair<std::string, std::string>> Pair_List;
    Pair_List GetSectionKeys(const std::string &_path);

    const std::set<std::string>& FilterImei() const
    {
        return filter_imei_;
    }
private:
    BF_KEY bf_key;
    std::set<std::string> filter_imei_;
    const std::string cfgfilename_;
    boost::property_tree::ptree cfgtree_;
    void Init(void);
};

}

}

#endif // CONFIGER_H
