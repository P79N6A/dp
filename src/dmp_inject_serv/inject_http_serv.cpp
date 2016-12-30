#include "inject_http_serv.h"

DEFINE_int32(http_port,18081,"http绑定端口");
DEFINE_int32(http_timeout,30,"http连接超时时间");
DEFINE_int32(http_worker_num,8,"http工作线程数");

namespace poseidon
{
namespace inject
{
  
  HttpServer::HttpServer()
	{
    _listen_fd=0;
	}
	HttpServer::~HttpServer()
	{
		
	}

	void HttpServer::startup()
	{
    _port=FLAGS_http_port;
		_http_time_out=FLAGS_http_timeout;
    _worker_num=FLAGS_http_worker_num;
		_listen_fd=::socket(AF_INET, SOCK_STREAM, 0);
    if(_listen_fd<=0)
    {
      cout<<"http server create false,listen_fd create error"<<endl;
      exit(-1);
    }

    evutil_make_listen_socket_reuseable(_listen_fd);
    
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(_port);

    int ret=::bind(_listen_fd, (struct sockaddr*)&sin, sizeof(sin));
    if(ret!=0)
    {
      cout<<"http server create false,bind error,"<<strerror(errno)<<endl;
      exit(-1);
    }
    ret=::listen(_listen_fd, 1024);
    if(ret!=0)
    {
      cout<<"http server create false,listen error,"<<strerror(errno)<<endl;
      exit(-1);
    }
    
    evutil_make_socket_nonblocking(_listen_fd);
    
    for(int i=0;i<_worker_num;i++)
    {
      HttpWorker * worker=new HttpWorker;
      worker->startup(_listen_fd,_http_time_out);
      _workers.push_back(worker);
    }
    
		running=true;
	}

}
}