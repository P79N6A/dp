/**
 * @company             
 */
#include "comm_event_timer.h"

namespace dc
{
namespace common
{
namespace comm_event
{

/** 
 * @brief               session timeout callback
 * @param
 * @return
 **/
int TimerBase::on_timeout()
{
	return 0;
}

int TimerBase::add_to_base(struct event_base * base)
{
	int rt=EC_SUCCESS;
	do{
	    if(base == NULL)
        {
    	    rt=EC_BASE_NULL;
    	    break;
        }
        base_=base;

        evtime_=evtimer_new(base_, timeout_callback, (void *)this);
        if(evtime_ == NULL)
        {
            rt=EC_EVTIME_NEW_ERR;
            break;
        }
        struct timeval tv;
        tv.tv_sec=timeout_/1000;
        tv.tv_usec=(timeout_-tv.tv_sec*1000)*1000;
        evtimer_add(evtime_, &tv);

	}while(0);
	return rt;
}

void TimerBase::timeout_callback(evutil_socket_t fd, short event, void * arg )
{
	TimerBase * pSess=(TimerBase *)arg;
	pSess->on_timeout();
}

}//comm_event
}//common
}//dc


