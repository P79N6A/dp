/**
 **/

#ifndef  _SESSION_MANAGER_H_ 
#define  _SESSION_MANAGER_H_
#include <vector>
#include <iostream>
#include "src/comm_event/comm_event_interface.h"
#include "src/comm_event/comm_event_factory.h"
#include "src/comm_event/comm_event_timer.h"
#include "boost/serialization/singleton.hpp"
#include "protocol/src/poseidon_proto.h"
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "api/exp_api.h"

namespace poseidon{
namespace control{

class SessionManager;

//store session data;
typedef struct
{
//    char reqbuf[8192];      //req form adapter
    std::string reqpack;        //req package
    rtb::BidRequest bidreq;     
    rtb::BidResponse bidrsp;
    qp::QPRequest qpreq;
    qp::QPResponse qprsp;
    sn::SNRequest snreq;
    sn::SNResponse snrsp;
    dn::DNRequest dnreq;
    dn::DNResponse dnrsp;
    ors::AlgoRequest orsreq;
    ors::AlgoResponse orsrsp;
    fc::FilterRequest filterreq;
    fc::FilterResponse filterrsp;
    feedback::FeedbackRequest fbreq;
    feedback::FeedbackResponse fbrsp;
    std::map<std::string, uint32_t> mapminprice;
    //std::map<uint64_t, common::Creative > mapcreative;
//    std::map<uint64_t, common::Ad > mapcad;//creativeid->ad;
    //std::map<uint32_t, common::FeedbackInfo> mapfeedback;//adgroupid->feedback;
    std::set<uint32_t> setfilter;   //adgroup_id be filtered
    std::string trace_id;

    int traffic_source;             //流量
    int prog;   //进度

    uint64_t time_bidreq;
    uint64_t time_qpreq;
    uint64_t time_qprsp;
    uint64_t time_snreq;
    uint64_t time_snrsp;
    uint64_t time_fcreq;
    uint64_t time_fcrsp;
    uint64_t time_fbreq;
    uint64_t time_fbrsp;
    uint64_t time_dnreq;
    uint64_t time_dnrsp;
    uint64_t time_orsreq;
    uint64_t time_orsrsp;
    uint64_t time_bidrsp;

    struct sockaddr_in client_addr; //client addr
    bool enabled_backup_ad;
    
    //试验系统
    std::vector<exp_sys::ExpParam> vr_exp_param;
    std::vector < int32_t > vr_exp_id;

}SessionData;

enum SESS_STAT
{
    STAT_NOSTART=0,
    STAT_GET_ADAPTER,       //recv request from adapter
    STAT_GET_QP,
    STAT_GET_SN,
    STAT_GET_FC,
    STAT_GET_FB,
    STAT_GET_DN,
    STAT_GET_ORS,
    STAT_FAIL,
    STAT_TIME_OUT,          //recv request from adapter
    STAT_ENABLE_BACKUP_AD,
};

enum SESS_PROG
{
    PROG_ADAPTER_REQ=1,
    PROG_QP_REQ=(1<<1),
    PROG_QP_RSP=(1<<2),
    PROG_SN_REQ=(1<<3),
    PROG_SN_RSP=(1<<4),
    PROG_FC_REQ=(1<<5),
    PROG_FC_RSP=(1<<6),
    PROG_FB_REQ=(1<<7),
    PROG_FB_RSP=(1<<8),
    PROG_DN_REQ=(1<<9),
    PROG_DN_RSP=(1<<10),
    PROG_ORS_REQ=(1<<11),
    PROG_ORS_RSP=(1<<12),
    PROG_ADAPTER_RSP=(1<<13),
};

//模板信息
struct TemplateInfo
{
    uint32_t template_id;   //模板ID
    uint32_t width;         //宽
    uint32_t height;        //高
};

class Session:public dc::common::comm_event::TimerBase 
{
public:

    enum EC
    {
        EC_SUCCESS=0,
    };

    Session():sid_(-1), manager_(NULL), status_(0){}

    void set_manager(SessionManager * manager)
    {
        manager_=manager;
    }

    /**
     * @brief       be called on session timeout
     * @return      success-0, or other errcode
     **/
    virtual int on_timeout();

    
    /**
     * @brief       be called on session be alloced
     * @return      success-0, or other errcode
     **/
    virtual int on_alloc();


    /**
     * @brief       be called on session be freeed
     * @return      success-0, or other errcode
     **/
    virtual int on_free();


    /**
     * @brief       放回Session的数据
     **/
    SessionData & data()
    {
        return data_;
    }

    void set_sid(int sid)
    {
        sid_=sid;
    }

    int sid()
    {
        return sid_;
    }

    int dispatch();

    /**
     * @brief       free self
     **/
    void free();

    int status()
    {
        return status_;
    }

    void status(int status)
    {
        status_=status;
    }


private:
    int sid_;                   //session id
    SessionManager * manager_;  //manager
    int status_;                //session's status
    SessionData data_;
};

class SessionManager:public boost::serialization::singleton<SessionManager>
{
public:
    
    SessionManager():init_(false), alloc_index_(0),time_out_(0),max_capacity_(0){}

    ~SessionManager(){}

    /**
     * @brief   capacity of SessionManager
     **/
    int init(int max_capacity);

    /**
     * @brief  set session timeout(ms)
     **/
    void set_session_timeout(int ms);


    /**
     * @brief  alloc session
     * @return  return session id, fail return -1
     **/
    int alloc_session();


    /**
     * @brief       get session from session id
     * @param sid   [IN], SID
     * @return  return the session, if session is invalid,return NULL
     **/
    Session * get_session(int sid);


    /**
     * @brief       free session
     * @param sid   [IN], session ID 
     **/
    void free_session(int sid);

private:
    bool init_;         //init or not
    int alloc_index_;   //alloc index
    int time_out_;      //timeout
    int max_capacity_;  //capacity
    std::vector<Session * > session_pool_; //session pool   
};

}//control
}//poseidon

#endif   // ----- #ifndef _SESSION_MANAGER_H_  ----- 
