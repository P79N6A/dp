#pragma once
#include "inject_inc.h"

namespace poseidon
{
namespace inject
{
  enum CmdType
  {
    CMD_GET_TAG=1,
    CMD_SET_TAG=2,
    CMD_DEL_TAG=3,
  };
  
  struct InjectContext
  {
    InjectContext(struct evhttp_request * req);
    virtual ~InjectContext();
    uint64_t id;
    struct evhttp_request *http_req;
    string content_type;
    string server_name;
    int http_code;
    string http_reason;
    struct evbuffer * send_data_buff;
    util::Timer timer;
    const struct evhttp_uri *uri;
    string uid;
    int tag_no;
    vector<int> tag_values;
    dmp::DmpUserData dmp_user_data;
    CmdType cmd_type;
    int prog;
    
    void set_http_ok();
    void set_http_internal_err();
    void set_http_nofound_err();
    string get_bucket_key();
    string get_bucket_index();
    void set_succ_res();
  };
}
}