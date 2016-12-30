/*
 * agent_client.h
 * Created on: 2016年10月25日
 */

#ifndef SRC_MEM_SYNC_AGENT_AGENT_CLIENT_H_
#define SRC_MEM_SYNC_AGENT_AGENT_CLIENT_H_

#include "event2/event.h"
#include "comm_event_factory.h"
#include "comm_event_tcp_connector.h"
#include "common/proto.h"

namespace poseidon {
namespace mem_sync {
namespace agent {

/* Agent Client represent a connection to the MemSync DataServer.
 * It process the MSReq protocol. The name is a bit weird to
 * distinct with DataClient, whatever.
 */

class AgentClient : public dc::common::comm_event::CommTcpConnector {
public:
	enum ReadState {
		UNREAD = 0,
		READ_HEADER = 1,
		READ_BODY = 2
	};

	AgentClient(int data_id, int version, const std::string& md5);
	virtual ~AgentClient();

	/* virtual functions from CommTcpBase, 0 for success. */
	virtual int init(int fd, int status);

	/* deserialize from buffer when the protocol data is ready. */
	virtual int handle_pkg(const char * buf, const int len);

	/* find that if we have read enough data. */
	virtual bool read_done(const char * buf, const int len);

	/* close socket if error occur or EOF is read. */
    virtual int on_error();

    virtual int on_close();

    virtual int on_timeout();

    virtual int on_connected();
protected:
    int MSRequest(MSReq &request);
    int OnMSResponse(MSRspHead &response, const char * buf, int size);
    int OnMSResponseData(const char * buf, int size);

private:
    int data_id_;
    int version_;
    std::string md5_;
    int read_stat_;
    int64_t mem_size_;
    int64_t mem_read_;
    void * mem_ptr_;
};

}}}

#endif /* SRC_MEM_SYNC_AGENT_AGENT_CLIENT_H_ */
