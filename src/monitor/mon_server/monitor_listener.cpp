/**
 **/


#include "monitor_listener.h"
#include "monitor_tcp.h"
#include "util/log.h"
#include "util/func.h"

namespace poseidon
{
namespace monitor
{

int MonitorListener::on_accept(evutil_socket_t fd, struct sockaddr *address, int socklen)
{
    LOG_DEBUG("on_accept called, client[%s]", util::Func::to_str(*(struct sockaddr_in *)address).c_str() );
    MonitorConn * conn=new MonitorConn();
    dc::common::comm_event::CommFactoryInterface::instance().add_comm_tcp(conn);
    conn->init(fd, STAT_CONNECTED);
    return 0;
}

int MonitorListener::on_accept_error()
{
    printf("%s called\n", __FUNCTION__);
    return 0;
}

}
}


