/*
 * agent_client.cpp
 * Created on: 2016-10-25
 */

#include <string>
#include "common/proto.h"
#include "util/log.h"
#include "util/func.h"
#include "config.h"
#include "agent_client.h"

#include "agent_attr.h"
#include "data_agent.h"
#include "mem_manager.h"
#include "monitor_api.h"

using poseidon::mem_sync::MemManager;
typedef poseidon::monitor::Api MonitorApi;

namespace poseidon {
namespace mem_sync {
namespace agent {

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

static bool ParseFromArray(MSRspHead & response, const char * buf)
{
	const char * p = buf;
	do {
		response.tag_begin = *p++;
		response.magic_num[0] = *p++;
		response.magic_num[1] = *p++;
		response.result = *(int *)p; p += 4;
		response.dataid = *(int *)p; p += 4;
		response.version = *(int *)p; p += 4; /* read from where? */
		response.size = *(int64_t *)p; p += 8; /* how many data are wanted */
		response.tag_end = *p++;
	} while (0);
	return true;
}

static bool DumpToString(MSReq &request, char * &buf)
{
	buf = new char[sizeof(MSReq)];
	if (!buf) return false;

	char * p = buf;
	do {
		*p++ = request.tag_begin;
		*p++ = request.magic_num[0];
		*p++ = request.magic_num[1];
		*(int *)p = request.dataid; p += 4;
		*(int *)p = request.version; p += 4;
		*(int64_t *)p = request.offset; p += 8;
		*(int64_t *)p = request.size; p += 8;
		*p = request.tag_end;
	} while (0);
	return true;
}

AgentClient::AgentClient(int data_id, int version, const std::string& md5) :
	data_id_(data_id), version_(version), md5_(md5),
	read_stat_(READ_HEADER), mem_ptr_(NULL)
{

}

AgentClient::~AgentClient(void)
{
	/* close connection */
	close_conn();
}

int AgentClient::init(int fd, int status)
{
	if (0 != dc::common::comm_event::CommTcpBase::init(fd, status)) {
		LOG_ERROR("AgentClient init failed!");
		return -1;
	}

	struct timeval tv;
	tv.tv_sec = Config::get_mutable_instance().AgentClientDataTimeout();
	tv.tv_usec = 0;
	bufferevent_set_timeouts(bev_, &tv, NULL);
	return 0;
}

bool AgentClient::read_done(const char * buf, const int len)
{
	if (read_stat_ == READ_HEADER) {
		return (unsigned int)len >= sizeof(MSRspHead) ? true : false;
	} else { // read_stat_ == READ_BODY
		return true;
	}
}

int AgentClient::handle_pkg(const char * buf, const int len)
{
	if (read_stat_ == READ_HEADER) {
		LOG_INFO("onHandlePackage, len = %d", len);
		MSRspHead response;
		ParseFromArray(response, buf);
		if (response.result != 0) {
			return OnMSResponse(response, NULL, 0);
		} else {
			read_stat_ = READ_BODY;
			return OnMSResponse(response, buf + sizeof(MSRspHead), len - sizeof(MSRspHead));
		}
	} else { // read_stat_ == READ_BODY
		return OnMSResponseData(buf, len);
	}
}

int AgentClient::OnMSResponse(MSRspHead &response, const char * buf, int len)
{
	mem_size_ = ntoh64(response.size);
	mem_read_ = 0;

	int data_id = ntohl(response.dataid);
	int version = ntohl(response.version);

	if (data_id_ != data_id || version != version_) {
		LOG_ERROR("dataid or version not match: agent ID(%d), server ID(%d) "
			"agent VER(%d), server VER(%d)",
			data_id_, data_id, version_, version);
		on_error();
		return -1;
	}

	MemManager &mgr = MemManager::get_mutable_instance();
	MonitorApi &mon = MonitorApi::get_mutable_instance();

	int res = mgr.update_data(data_id, version_, (uint64_t)mem_size_, mem_ptr_);
	if (0 != res) {
		LOG_WARN("MemManager UpdateData failed! res = %d, "
		    "ID = %d, VER = %d, size = %lld",
			res, data_id_, version_, mem_size_);

		mon.mon_add(ATTR_DATA_AGENT_UPDATE_FAIL, 1);
		on_error();
	}

	return OnMSResponseData(buf, len);
}

/* write to memory manager,
 * @return: 0 for success, otherwise failed.
 */
int AgentClient::OnMSResponseData(const char * data, int size)
{
	// LOG_INFO("received size = %d, total = %d", size, mem_read_);

	memcpy((void *)(mem_ptr_ + mem_read_), (const void *)data, size);
	mem_read_ += size;

	if (mem_read_ == mem_size_) {
		/* check MD5 */
		DataAgent &agent = DataAgent::get_mutable_instance();
		MemManager &mgr = MemManager::get_mutable_instance();
		MonitorApi &mon = MonitorApi::get_mutable_instance();

		int res = -1;
		int stat = DataAgent::PENDING;
		do {
			/* can only check MD5 less than MAX_UINT */
			if (mem_read_ < (uint32_t)(-1)) {
				std::string checksum;
				util::Func::md5sum((char *)mem_ptr_, (uint32_t)mem_read_, checksum);

				if (checksum != md5_) {
					LOG_WARN("MD5 not match, drop packet dataid(%d), version(%d), md5(%s), serverMD5(%s)",
						data_id_, version_, md5_.c_str(), checksum.c_str());

					stat = DataAgent::BAD;
					mon.mon_add(ATTR_DATA_AGENT_MD5_FAIL, 1);
					break;
				} else {
				    /* monitor doesn't support int64 */
				    mon.mon_add(ATTR_DATA_AGENT_DATA_TRANSMITTED, (int)mem_read_);
				}
			} else {
			    mon.mon_add(ATTR_DATA_AGENT_DATA_TRANSMITTED, (int)mem_read_);
			}

			LOG_INFO("sync data from server finished: %d bytes total", mem_size_);
			res = mgr.update_done(data_id_);
			if (res != 0) {
				LOG_WARN("MemManager UpdateDone failed! res = %d "
				    "ID = %d, VER = %d, size = %lld",
					res, data_id_, version_, mem_size_);

				/* dont' fetch again ... */
				stat = DataAgent::BAD;
				mon.mon_add(ATTR_DATA_AGENT_UPDATE_FAIL, 1);
				break;
			}

			LOG_INFO("update done ... OK");
			stat = 0;
		} while (0);

		agent.UpdateMemStat(data_id_, version_, stat);
		delete this;
	}
	return 0;
}

int AgentClient::on_error(void)
{
	LOG_DEBUG("AgentClient on_error");

	/* set stat to pending to wait for the next schedule */
	DataAgent & agent = DataAgent::get_mutable_instance();
	agent.UpdateMemStat(data_id_, version_, DataAgent::PENDING);

	delete this;
	return 0;
}

int AgentClient::on_close(void)
{
	LOG_DEBUG("AgentClient on_close");
	DataAgent & agent = DataAgent::get_mutable_instance();
	agent.UpdateMemStat(data_id_, version_, DataAgent::PENDING);

	delete this;
	return 0;
}

int AgentClient::on_connected(void)
{
    LOG_INFO("AgentClient on_connect");
	MSReq request;
	request.dataid = ntohl(data_id_);
	request.version = ntohl(version_);
	request.size = 0;
	request.offset = 0;
	request.tag_begin = '(';
	request.tag_end = ')';
	request.magic_num[0] = 'M';
	request.magic_num[1] = 'S';

	char * buf;
	DumpToString(request, buf);
	write(buf, sizeof(MSReq));
	delete[] buf;
	return 0;
}

int AgentClient::on_timeout(void)
{
	LOG_WARN("AgentClient DataTimeout. ");
	return on_error();
}

}}}







