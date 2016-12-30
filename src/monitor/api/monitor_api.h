/**
 **/


#ifndef  _MONITOR_API_H_ 
#define  _MONITOR_API_H_

#include <boost/serialization/singleton.hpp>
#include "../common/define.h"

namespace poseidon
{
namespace monitor
{

class Api:public boost::serialization::singleton<Api>
{
public:

    Api():init_(false)
    {
    }

    /**
     * @brief       初始化
     **/
    int init();

    /**
     * @brief       属性上报add
     **/
    void mon_add(int attr_id, int val);

    /**
     * @brief       属性上报set
     **/
    void mon_set(int attr_id, int val);

    void update_index(Attr & attr);

private:

    bool init_;                     //是否初始化
    MonitorLayOut * layout_;        //内存布局

};//Api
}//monitor
}//poseidon

#define MON_ADD(attr, val) poseidon::monitor::Api::get_mutable_instance().mon_add(attr, val)
#define MON_SET(attr, val) poseidon::monitor::Api::get_mutable_instance().mon_set(attr, val)

#endif   // ----- #ifndef _MONITOR_API_H_  ----- 

