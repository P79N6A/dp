#include "logcheck_worker.h"

namespace poseidon {
namespace log_check {

void udp_recv_cb(evutil_socket_t fd, short event, void *arg) {
    if (!(event & EV_READ))
        return;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[UDP_SERV_RECV_BUFFER_SIZE] = { 0 };
    int recv_len = recvfrom(fd, buffer, sizeof(buffer), 0,
            (struct sockaddr *) &addr, &addr_len);
    if (recv_len < 0) {
        return;
    }

    LogCheckWorker * worker = (LogCheckWorker *) arg;
    worker->process(buffer, recv_len, addr);
}


void LogCheckWorker::start_up(int sock_fd) {
    _sock_fd = sock_fd;
    _base = event_base_new();
    if (_base == NULL) {
        LOG_ERROR("worker start up error,event can not new");
        exit(-1);
    }


    struct event *udp_recv_event = new event;
    event_assign(udp_recv_event, _base, _sock_fd, EV_PERSIST | EV_READ,
            udp_recv_cb, (void *) this);
    event_add(udp_recv_event, NULL);

    event_base_dispatch(_base);
}

void LogCheckWorker::process(char *recv_buf, int recv_len,
        const struct sockaddr_in &addr) {
  LOG_DEBUG("recv log : %s",recv_buf);
  string log_str;
  log_str.assign(recv_buf,recv_len);
  vector<string> lines;
  boost::split(lines, log_str, boost::is_any_of("`"));
  boost::unordered_map<string,string> kvs;
  for(int i=0;i<lines.size();i++)
  {
    if(lines[i].length()<3)
      continue;
    vector<string> kv;
    boost::split(kv, lines[i], boost::is_any_of("="));
    if(kv.size()<2)
    {
      LOG_ERROR("[NULL VALUE][%s]",log_str.c_str());
      LOG_ERROR("%s has not value",lines[i].c_str());
      MON_ADD(LOG_CHECK_ERROR_NULL_VALUE, 1);
      return;
    }
    if(kvs.count(kv[0])>0)
    {
      LOG_ERROR("[MULTI KEY][%s]",log_str.c_str());
      LOG_ERROR("%s is multi key",kv[0].c_str());
      MON_ADD(LOG_CHECK_ERROR_MULTI_KEY, 1);
      return;
    }
    kvs[kv[0]]=kv[1];
  }
  if(kvs.count("trace_id")==0)
  {
    kvs["trace_id"]="null";
  }
  int chk_ret=pv_rules_.check(kvs);
  if(chk_ret<0)
  {
    LOG_ERROR("[%d][%s]",chk_ret,log_str.c_str());
  }
  os_rules_.check(kvs);
}

}
}
