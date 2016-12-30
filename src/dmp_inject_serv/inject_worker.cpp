#include "inject_worker.h"

DEFINE_string(tag_redis_host,"127.0.0.1","redis host");
DEFINE_int32(tag_redis_port,6381,"redis port");
DEFINE_bool(tag_redis_cluster,false,"是否使用redis集群");

DEFINE_string(get_path,"/get","get_path");
DEFINE_string(set_path,"/set","set_path");
DEFINE_string(del_path,"/del","del_path");
namespace poseidon
{
namespace inject
{
  
  static bool get_para_from_uri(boost::shared_ptr<InjectContext> context,const string & key,string & value)
  {
    bool ret=false;
    const char * qstring=evhttp_uri_get_query(context->uri);
    do
    {
      if(qstring==NULL)
      {
        break;
      }
      struct evkeyvalq params;
      if(evhttp_parse_query_str(qstring,&params)!=0)
      {
        break;
      }
      const char *pvalue=evhttp_find_header(&params,key.c_str());
      if(pvalue==NULL)
      {
        break;
      }
      if(strlen(pvalue)==0)
      {
        break;
      }
      value=pvalue;
      ret=true;
    }while(0);
    return ret;
  }
  void http_connection_close_cb(evhttp_connection * conn,void * arg)
	{
		char * peer_addr=0;
		uint16_t peer_port=0;
		evhttp_connection_get_peer(conn,&peer_addr,&peer_port);
	}
	
	void http_request_cb(struct evhttp_request *req, void *arg)
	{	
		struct evhttp_connection * conn=evhttp_request_get_connection(req);
		evhttp_connection_set_closecb(conn,http_connection_close_cb,NULL);
		HttpWorker * worker=(HttpWorker *)arg;
		worker->recv_request(req);
	}

	HttpWorker::HttpWorker()
	{
    _serial_num=0;
    _fd=0;
    _httpd=NULL;
	}
	HttpWorker::~HttpWorker()
	{
		
	}

	void HttpWorker::startup(evutil_socket_t fd,int time_out)
	{
    _fd=fd;
    _http_time_out=time_out;
		_base = event_base_new();

		_httpd = evhttp_new(_base);
		evhttp_set_timeout(_httpd,_http_time_out);
		evhttp_accept_socket(_httpd,_fd);
		evhttp_set_gencb(_httpd, http_request_cb, (HttpWorker*)this);
		util::Closure * run_closure=util::NewCallback(this,&HttpWorker::run);
		util::ThreadPool::get_mutable_instance().add_task(run_closure);
	}
	
	void HttpWorker::run()
	{
    _redis_client.init(_base,FLAGS_tag_redis_host,FLAGS_tag_redis_port,FLAGS_tag_redis_cluster);
		event_base_dispatch(_base);
	}
	
	void HttpWorker::recv_request(struct evhttp_request *req)
	{
    boost::shared_ptr<InjectContext> context(new InjectContext(req));
    context->id=_serial_num++;
    _context_map[context->id]=context;
    process(context);
	}
	
  void HttpWorker::process(boost::shared_ptr<InjectContext> context)
  {
    
    string path=evhttp_uri_get_path(context->uri);
    if(path.compare(FLAGS_get_path)==0)
    {
      get_process(context);
      return;
    }
    else if(path.compare(FLAGS_set_path)==0)
    {
      set_process(context);
      return;
    }
    else if(path.compare(FLAGS_del_path)==0)
    {
      del_process(context);
      return;
    }
    else
    {
      context->set_http_nofound_err();
      http_done(context);
    }
  }
  
  void HttpWorker::del_process(boost::shared_ptr<InjectContext> context)
  {
    if(!get_para_from_uri(context,"uid",context->uid))
    {
      context->set_http_internal_err();
      http_done(context);
      return;
    }
    string tag_no;
    get_para_from_uri(context,"tag_no",tag_no);
    context->tag_no=strtoul(tag_no.c_str(),NULL,10);
    context->cmd_type=CMD_DEL_TAG;
    schedule(context);
  }
  
  void HttpWorker::set_process(boost::shared_ptr<InjectContext> context)
  {
    string tag_no;
    string tag_values;
    if( !get_para_from_uri(context,"uid",context->uid) || 
        !get_para_from_uri(context,"tag_no",tag_no) ||
        !get_para_from_uri(context,"tag_values",tag_values) )
    {
      context->set_http_internal_err();
      http_done(context);
      return;
    }
    context->tag_no=strtoul(tag_no.c_str(),NULL,10);
    vector<string> columns;
    boost::split(columns, tag_values, boost::is_any_of(",")); 
    for(int i=0;i<columns.size();i++)
    {
      int value=strtoul(columns[i].c_str(),NULL,10);
      context->tag_values.push_back(value);
    }
    context->cmd_type=CMD_SET_TAG;
    schedule(context);
  }
  
  void HttpWorker::get_process(boost::shared_ptr<InjectContext> context)
  {
    if(!get_para_from_uri(context,"uid",context->uid))
    {
      context->set_http_internal_err();
      http_done(context);
      return;
    }
    context->cmd_type=CMD_GET_TAG;
    schedule(context);
  }
  
  void HttpWorker::schedule(boost::shared_ptr<InjectContext> context)
  {
    if(context->http_code!=0)
    {
      context->set_http_internal_err();
      http_done(context);
    }
    if(context->cmd_type==CMD_GET_TAG)
    {
      if(context->prog==0)
        get_user_tags(context);
      else
      {
        context->set_succ_res();
        http_done(context);
      }
    }
    else if(context->cmd_type==CMD_SET_TAG)
    {
      if(context->prog==0)
        get_user_tags(context);
      else if(context->prog==1)
      {
        dmp::DmpUserData::TagData * tag_data=NULL;
        for(int i=0;i<context->dmp_user_data.tag_datas_size();i++)
        {
          dmp::DmpUserData::TagData * tmp=context->dmp_user_data.mutable_tag_datas(i);
          if(tmp->tag_no()==context->tag_no)
          {
            tmp->clear_values();
            tag_data=tmp;
            break;
          }
        }
        if(tag_data==NULL)
        {
          tag_data=context->dmp_user_data.add_tag_datas();
          tag_data->set_tag_no(context->tag_no);
        }
        for(int i=0;i<context->tag_values.size();i++)
        {
          tag_data->add_values(context->tag_values[i]);
        }
        set_user_tag(context);
      }
      else
      {
        context->set_succ_res();
        http_done(context);
      }
    }
    else if(context->cmd_type==CMD_DEL_TAG)
    {
      if(context->prog==0)
      {
        if(context->tag_no!=0)
          get_user_tags(context);
        else
        {
          del_user_tags(context);
        }
      }
      else if(context->prog==1)
      {
        dmp::DmpUserData tmp_user_data;
        for(int i=0;i<context->dmp_user_data.tag_datas_size();i++)
        {
          const dmp::DmpUserData::TagData &tmp1=context->dmp_user_data.tag_datas(i);
          if(tmp1.tag_no()!=context->tag_no)
          {
            dmp::DmpUserData::TagData * tmp2=tmp_user_data.add_tag_datas();
            tmp2->CopyFrom(tmp1);
          }
        }
        context->dmp_user_data.CopyFrom(tmp_user_data);
        set_user_tag(context);
      }
      else
      {
        context->set_succ_res();
        http_done(context);
      }
    }
    else
    {
      context->set_http_internal_err();
      http_done(context);
    }
  }
  
	void HttpWorker::http_done(boost::shared_ptr<InjectContext> context)
	{
    ContextIter iter=_context_map.find(context->id);
    if(iter==_context_map.end())
      return;
    _context_map.erase(iter);
    struct evhttp_request *req=context->http_req;
    evhttp_add_header(req->output_headers, "Content-Type", context->content_type.c_str());
    evhttp_add_header(req->output_headers, "Server", context->server_name.c_str());
    evhttp_send_reply(req,context->http_code,
    context->http_reason.c_str(),context->send_data_buff);
	}

}
}