/**
 * @company             
 */

#ifndef  COMM_EVENT_TIMER_H_
#define  COMM_EVENT_TIMER_H_

extern "C"
{
#include "event2/event.h"
}

#include "comm_event_interface.h"

namespace dc
{
namespace common
{
namespace comm_event
{

class TimerBase:public CommTimerInterface
{

public:
    TimerBase():base_(NULL), evtime_(NULL), timeout_(1000)
    {
    }

    virtual ~TimerBase()
    {
    	if(evtime_ != NULL)
        {
            evtimer_del(evtime_);
            event_free(evtime_);
        }
    }


    /** 
     * @brief               set timeout time(ms)
     * @param
     * @return
     **/
    virtual int set_timeout(int timeout)
    {
        timeout_=timeout;
        return 0;
    }


    /** 
     * @brief               timeout callback
     * @param
     * @return
     **/
    virtual int on_timeout();


    static void timeout_callback(evutil_socket_t fd, short event, void * arg );

    int add_to_base(struct event_base * base);
private:
    struct event_base * base_;          /*base*/
    struct event * evtime_;             /*¶¨Ê±Æ÷*/
    int timeout_;                       /*timeout ms*/

};

}//comm_event
}//common
}//dc




#endif   /* ----- #ifndef COMM_EVENT_TIMER_H_  ----- */

