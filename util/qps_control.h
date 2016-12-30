/**
 **/

#ifndef  _QPS_CONTROL_H_ 
#define  _QPS_CONTROL_H_

#include <boost/circular_buffer.hpp>

#include "util/func.h"

namespace poseidon
{
namespace util
{

class QpsControl
{
public:

    QpsControl():cb_time_(NULL)
    {
    }

    virtual ~QpsControl()
    {
    }
    
    /**
     * @brief           设置最大的Qps
     **/
    int set_max_qps(int max_qps)
    {
        if(cb_time_ != NULL)
        {
            delete cb_time_;
        }
        cb_time_ = new(std::nothrow) boost::circular_buffer<uint64_t>(max_qps);
        if(cb_time_ == NULL)
        {
            return -1;
        }else
        {
            return 0;
        }
    }

    bool allow()
    {
        if(cb_time_==NULL)
        {
            //没有调用set_max_qps，认为不做qps控制
            return true;
        }

        uint64_t now;
        util::Func::get_time_ms(now);
        if(!cb_time_->full())
        {
            cb_time_->push_back(now);
            return true;
        }else
        {
            uint64_t front=cb_time_->front();
            if(now-front<=1000)
            {
                return false;
            }else
            {
                cb_time_->push_back(now);
                return true;
            }
        }
    }

private:
    boost::circular_buffer<uint64_t> * cb_time_;    //时间

};

}
}

#endif   // ----- #ifndef _QPS_CONTROL_H_  ----- 


