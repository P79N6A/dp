/**
 * @company             
 */

#ifndef  COMM_EVENT_COMM_EVENT_TCP_CONNECTOR_H_
#define  COMM_EVENT_COMM_EVENT_TCP_CONNECTOR_H_

#include "comm_event_tcp.h"

namespace dc
{
namespace common
{
namespace comm_event
{

class CommTcpConnector: public CommTcpBase
{
public:
    /** 
     * @brief               �󶨱��ض˿�
     * @return              �ɹ�����0��ʧ�ܷ���-1
     **/
    virtual int bindaddr(const std::string ip, const uint16_t port );

    
    /** 
     * @brief               connectһ��Զ�̵�ַ
     **/
    virtual int connect(struct sockaddr_in & dst_addr);

};

}
}
}
#endif   /* ----- #ifndef COMM_EVENT_COMM_EVENT_TCP_CONNECTOR_H_  ----- */

