/*
 * data_client.h
 * Created on: 2016-10-20
 */

#ifndef AGENT_CLIENT_H_
#define AGENT_CLIENT_H_

#include "event2/event.h"
#include "comm_event_factory.h"
#include "comm_event_tcp.h"
#include "common/proto.h"

namespace poseidon {
namespace mem_sync {
namespace server {

/* Data Client represent a connection to the MemSync Data Server.
 * It process the MSReq protocol
 * */
class DataClient : public dc::common::comm_event::CommTcpBase {
public:
	DataClient();
	virtual ~DataClient();

	/* virtual functions from CommTcpBase, 0 for success. */
	virtual int init(int fd, int status);

	/* deserialize from buffer when the protocol data is ready. */
	virtual int handle_pkg(const char * buf, const int len);

	/* find that if we have read enough data. */
	virtual bool read_done(const char * buf, const int len);

	/* close socket if error occur or EOF is read. */
    virtual int on_error();

    virtual int on_close();

    virtual void on_timer();

    virtual int write(const char * buf, const int len);

    static void timer_callback(evutil_socket_t fd, short event, void * arg);
protected:
    int OnMSRequest(MSReq &request);
    int MSResponse(MSRspHead &response, const char * data, int64_t size);

private:
    struct event * timer_;
    int max_token_;
    int cur_token_;
    int cir_; /* Committed information rate */
    int tc_;  /* time interval */
    int bc_;  /* bursr size */
    const char * ptr_; /* cache shm's pointer*/
    int psize_;  /* how many data left */
    int dataid_;
    int version_;
};

}}}

#endif /* AGENT_CLIENT_H_ */
