/**
 * @company             
 */

#include "comm_event_timer.h"
#include "comm_event_interface.h"
#include "comm_event_factory.h"

class CTimerTest:public dc::common::comm_event::TimerBase
{
public:
    /** 
     * @brief               timeout callback
     * @param
     * @return
     **/
    int on_timeout()
    {
    	printf("%s called!\n", __FUNCTION__ );
        dc::common::comm_event::CommTimerInterface * timer=new CTimerTest(); 

        timer->set_timeout(2000);

        dc::common::comm_event::CommFactoryInterface::instance().add_comm_timer(timer);

        delete this;
        return 0;
    }

};
int main()
{
    if(dc::common::comm_event::CommFactoryInterface::instance().init() != dc::common::comm_event::EC_SUCCESS)
    {
        printf("CommFactoryInterface::instance().init() return error\n");
    }

    dc::common::comm_event::CommTimerInterface * timer=new CTimerTest(); 

    timer->set_timeout(2000);

    dc::common::comm_event::CommFactoryInterface::instance().add_comm_timer(timer);

    dc::common::comm_event::CommFactoryInterface::instance().run();


}
