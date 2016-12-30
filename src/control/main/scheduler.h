/**
 **/


#include "boost/serialization/singleton.hpp"
#include "session_manager.h"
#include "util/qps_control.h"

namespace poseidon
{
namespace control
{

enum ControlComm
{
  CONTROL_EXP_MODULE_ID=10001,
};

enum PvLogConnType
{
  PV_LOG_CONN_TYPE_UNKOWN=0,
  PV_LOG_CONN_TYPE_WIFI=1,
  PV_LOG_CONN_TYPE_2G=2,
  PV_LOG_CONN_TYPE_3G=3,
  PV_LOG_CONN_TYPE_4G=4
};

class Scheduler: public boost::serialization::singleton<Scheduler>
{
public:
    Scheduler()
    {
      serial_num_=0;
    }
    enum
    {
        NBR_SUCCESS=0,
        NBR_TECHNICAL_ERROR=4,
    };

    enum TS
    {
        JS_PT=0,
        TANX=1,
    };

    /**
     * @brief           调度session
     **/
    int dispatch(Session * sess);


    /**
     * @brief           called when get Adapter Req after
     **/
    int process_adapter_get(Session * sess);

    /**
     * @brief           called when get Qp Rsp after
     **/
    int process_qp_get(Session * sess);


    /**
     * @brief           called when get Sn Rsp after
     **/
    int process_sn_get(Session * sess);



    /**
     * @brief           called when get fb Rsp after
     **/
    int process_fb_get(Session * sess);

    /**
     * @brief            called when get Dn Rsp after 
     **/
    int process_dn_get(Session * sess);

    
    /**
     * @brief            called when get Ors Rsp after 
     **/
    int process_ors_get(Session * sess);

    /**
     * @brief            called on fail
     **/
    int process_fail(Session * sess);

    /**
     * @brief           called on session timeout
     **/
    int process_timeout(Session * sess);


    /**
     * @brief          build qp req 
     **/
    int build_qp_req(Session * sess);


    /**
     * @brief           build sn req
     **/
    int build_sn_req(Session *sess);


    /**
     * @brief           build adz info
     **/
    int build_adz_info(Session *sess);


    /**
     * @brief
     **/
    int build_time_info(Session * sess);


    /**
     * @brief       
     **/
    int build_geo(Session * sess);



    /**
     * @brief           build fc req
     **/
    int build_fb_req(Session *sess);

    /**
     * @brief           build dn req
     **/
    int build_dn_req(Session *sess);


    int build_adapter_rsp(Session * sess);

    int build_error_adapter_rsp(Session * sess);

    /**
     * @brief           build ors req
     **/
    int build_ors_req(Session *sess);
    
    int ors_build_traffic_info(Session * sess);

    int ors_build_user_info(Session * sess);

    int ors_build_device_info(Session * sess);

    int ors_build_algo_ad(Session * sess);

    int ors_build_exp(Session * sess);
    
    void write_log(Session * sess);

    void status_bid_latency(uint64_t latency);
    
    void status_qp_latency(uint64_t latency);
    void status_sn_latency(uint64_t latency);
    void status_fb_latency(uint64_t latency);
    void status_dn_latency(uint64_t latency);
    void status_ors_latency(uint64_t latency);

    int set_max_qps(int max_qps )
    {
        if(max_qps >= 0)
        {
            return qpscontrol_.set_max_qps(max_qps);
        }else
        {
            return -1;
        }
    }

private:
    uint64_t serial_num_;
    util::QpsControl qpscontrol_;   //流量控制
    bool enable_backup_ad(Session * sess);  //是否需要启用兜底广告
};

}
}
