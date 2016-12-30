#include "inject_inc.h"
#include "inject_http_serv.h"
using namespace poseidon;

DEFINE_string(conf, "../etc/conf.cfg", "配置文件路径");
DEFINE_string(log_conf,"../etc/log4cpp.conf","日志配置文件路径");
DEFINE_string(log_category,"inject_serv","日志类目设置,请不要更改");
DEFINE_string(pid,"../run.pid","pid文件路径");

int main(int argc, char** argv)
{
  google::SetVersionString("1.0.14");
  ::google::ParseCommandLineFlags(&argc, &argv, true);
  ::google::ReadFromFlagsFile(FLAGS_conf,argv[0],true);
  
  if(!util::Func::single_instance(FLAGS_pid))
  {
    fprintf(stderr, " %s already run...\n",argv[0]);
    exit(-1);
  }
  
  if(!LOG_INIT(FLAGS_log_conf,FLAGS_log_category))
  {
      fprintf(stderr, "LOG_INIT error[%s, %s]\n",FLAGS_log_conf.c_str(),FLAGS_log_category.c_str());
      return -1;
  }
  LOG_INFO("LOG_INIT SUCCESS!");
  
  inject::HttpServer::get_mutable_instance().startup();
  
  while(true)
  {
    LOG_INFO("inject.serv running...");
    sleep(10);
  }
  
}