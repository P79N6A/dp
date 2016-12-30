/*
 * data_client.cpp
 * Created on: 2016-10-20
 */

#include <string>
#include "common/proto.h"
#include "util/log.h"
#include "config.h"
#include "memory_mgr.h"
#include "data_client.h"

namespace poseidon {
namespace mem_sync {
namespace server {

static int64_t ntoh64(int64_t value)
{
	typedef union {
		char c[8];
		int64_t i;
	} data;

	data d;
	d.c[0] = *(((char *)&value) + 7);
	d.c[1] = *(((char *)&value) + 6);
	d.c[2] = *(((char *)&value) + 5);
	d.c[3] = *(((char *)&value) + 4);
	d.c[4] = *(((char *)&value) + 3);
	d.c[5] = *(((char *)&value) + 2);
	d.c[6] = *(((char *)&value) + 1);
	d.c[7] = *(((char *)&value) + 0);
	return d.i;
}

#define hton64(i) ntoh64((i))

static bool ParseFromArray(MSReq & request, const char * buf, int len)
{
	const char * p = buf;
	do {
		request.tag_begin = *p++;
		request.magic_num[0] = *p++;
		request.magic_num[1] = *p++;
		request.dataid = *(int *)p; p += 4;
		request.version = *(int *)p; p += 4;
		request.offset = *(int64_t *)p; p += 8; /* read from where? */
		request.size = *(int64_t *)p; p += 8; /* how many data are wanted */
		request.tag_end = *p++;
	} while (0);
	return true;
}

static bool DumpToString(MSRspHead &response, char * &buf)
{
	buf = new char[sizeof(MSRspHead)];
	if (!buf) return false;

	char * p = buf;
	do {
		*p++ = response.tag_begin;
		*p++ = response.magic_num[0];
		*p++ = response.magic_num[1];
		*(int *)p = response.result; p += 4;
		*(int *)p = response.dataid; p += 4;
		*(int *)p = response.version; p += 4;
		*(int64_t *)p = response.size; p += 8;
		*p = response.tag_end;
	} while (0);
	return true;
}

DataClient::DataClient() : timer_(NULL), ptr_(NULL)
{

}

DataClient::~DataClient(void)
{
	if (ptr_) {
		MemoryMgr::get_mutable_instance().ReleaseData(dataid_, version_);
		ptr_ = NULL;
	}

	if (timer_) {
		event_del(timer_);
		event_free(timer_);
	}
}

/* FIXME: read_done and handle_pkg should be fixed later.
 * handle_pkg should return the bytes that we are handled.
 *
 * here we use flow control so if flow control is in used,
 * we should consider read_done to be false.
 */
bool DataClient::read_done(const char * buf, const int len)
{
	LOG_INFO("sizeof(MSReq) = %d", sizeof(MSReq));
	return (unsigned int)len >= sizeof(MSReq) && !ptr_ ? true : false;
}

int DataClient::handle_pkg(const char * buf, const int len)
{
	LOG_INFO("onHandlePackage, len = %d", len);
	MSReq request;
	ParseFromArray(request, buf, len);

	return OnMSRequest(request);
}

int DataClient::OnMSRequest(MSReq &request)
{
	MSRspHead response;
	int64_t size;
	int64_t offset = ntoh64(request.offset);
	int64_t rsize = ntoh64(request.size);

	dataid_ = ntohl(request.dataid);
	version_ = ntohl(request.version);

	response.tag_begin = '(';
	response.tag_end = ')';
	response.magic_num[0] = 'M';
	response.magic_num[1] = 'S';
	response.dataid = request.dataid;
	response.version = request.version;
	response.size = 0;

	if (request.tag_begin != '(' ||
		request.tag_end != ')' ||
		request.magic_num[0] != 'M' ||
		request.magic_num[1] != 'S') {

		/* not valid ... */
		response.result = -1;
		return MSResponse(response, NULL, 0);
	}

	const char * ptr = poseidon::mem_sync::server::MemoryMgr::get_mutable_instance().GetData(
		ntohl(request.dataid),
		ntohl(request.version),
		offset,
		size);

	if (ptr) {
		if (offset + rsize > size) {
			LOG_WARN("size error: request offset = %d, size = %d, shmsize = %d",
				offset, rsize, size);
			ptr = 0; /* set to NULL for invalid case */
		}
		LOG_INFO("data ID(%d), VER(%d) found", dataid_, version_);
	} else {
		LOG_INFO("data ID(%d), VER(%d) not found", dataid_, version_);
	}

	response.result = ptr ? 0 : -1;
	response.size = hton64(size);
	MSResponse(response, ptr, size);
	return 0;
}

/* Response to agent, add rate limit logic here. */
int DataClient::MSResponse(MSRspHead &response, const char * data, int64_t size)
{
	char * buf;
	if (!DumpToString(response, buf)) {
		LOG_ERROR("not enough memory");
		return close_conn();
	}

	write(buf, sizeof(MSRspHead));
	delete[] buf;

	/* enable flow control */
	ptr_ = data;
	psize_ = size;

	if (ptr_) {
	    bufferevent_disable(bev_, EV_READ);
	    write(ptr_, psize_);
	}
	return 0;
}

/* statc method */
void DataClient::timer_callback(evutil_socket_t fd, short event, void * arg)
{
	// LOG_INFO("on timer");
	DataClient * client = (DataClient *)arg;
	client->on_timer();
}

void DataClient::on_timer()
{
	cur_token_ += bc_;
	if (cur_token_ > max_token_) {
		cur_token_ = max_token_;
	}

	if (ptr_) { /* we are writing ... */
		write(ptr_, psize_);
	}
}

int DataClient::write(const char * buf, int size)
{
	if (!ptr_)
		return dc::common::comm_event::CommTcpBase::write(buf, size);

	/* flow control enabled */
	if (size > cur_token_) {
		dc::common::comm_event::CommTcpBase::write(buf, cur_token_);
		psize_ -= cur_token_;
		ptr_ += cur_token_;
		cur_token_ = 0;
	} else {
		dc::common::comm_event::CommTcpBase::write(buf, size);
		psize_ -= size;
		ptr_ += size;
		cur_token_ -= size;
	}

	if (!psize_) {
		/* we are done, set ptr_ to 0 */
		ptr_ = 0;
		MemoryMgr::get_mutable_instance().ReleaseData(dataid_, version_);
		bufferevent_enable(bev_, EV_READ);
	}
	return 0;
}

int DataClient::init(int fd, int status)
{
	if (0 != dc::common::comm_event::CommTcpBase::init(fd, status)) {
		LOG_WARN("Data Client init failed!");
		/* should we delete this? */
		delete this;
		return -1;
	}

	/* init CIR, TC, BC */
	Config &config = Config::get_mutable_instance();
	max_token_ = config.FlowMaxToken();
	cur_token_ = 0;
	cir_ = config.FlowCIR();
	tc_ = config.FlowTC();
	bc_ = cir_ * tc_ / 1000;

	/* init timer event */
	timer_ = event_new(base_, -1, EV_PERSIST, timer_callback, (void *)this);
	if (timer_ == NULL) {
		LOG_WARN("Can not create timer event");
		delete this;
		return -1;
	}

	struct timeval tv;
	tv.tv_sec = tc_ / 1000;
	tv.tv_usec= tc_ * 1000 % 1000000;
	// LOG_INFO("tc_ = %d, sec = %d, usec = %d", tc_, tv.tv_sec, tv.tv_usec);
	event_add(timer_, &tv);
	return 0;
}

int DataClient::on_error()
{
	LOG_DEBUG("DataClient on_error");
	close_conn();
	delete this;
	return 0;
}

int DataClient::on_close()
{
	LOG_DEBUG("DataClient on_close");
	close_conn();
	delete this;
	return 0;
}

}}}




