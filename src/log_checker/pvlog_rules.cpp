#include "pvlog_rules.h"

DEFINE_string(pvlog_sources, "1,2,3,4,5,6,7", "pvlog source枚举");
DEFINE_string(check_pid_sources, "2,6,7", "检查pid的渠道");
DEFINE_int32(max_minute_pids,64, "某渠道一分钟pid最大值");

namespace poseidon {
namespace log_check {

int MinutePidNum::set(const string & pid)
{
  int now_minute=time((time_t)NULL)/60;
  if(now_minute>minute_)
    pids_.clear();
  if(pids_.size()<FLAGS_max_minute_pids*2)
    pids_.insert(pid);
  return pids_.size();
}

void PvLogRules::init()
{
  if(is_init_)
    return;
  vector<string> lines;
  boost::split(lines, FLAGS_pvlog_sources, boost::is_any_of(","));
  for(int i=0;i<lines.size();i++)
    source_set_.insert(lines[i]);
  lines.clear();
  boost::split(lines, FLAGS_check_pid_sources, boost::is_any_of(","));
  for(int i=0;i<lines.size();i++)
  {
    MinutePidNum mpn;
    minute_pid_num_map_[lines[i]]=mpn;
  }
  is_init_=true;
}

int PvLogRules::check(boost::unordered_map<string,string> & kvs)
{
  init();
  if(kvs.count("source")==0)
  {
    MON_ADD(LOG_CHECK_ERROR_NO_SOURCE, 1);
    return PVLOG_CHECK_NO_SOURCE;
  }
  string &source=kvs["source"];
  if(source_set_.count(source)==0)
  {
    MON_ADD(LOG_CHECK_ERROR_ERROR_SOURCE, 1);
    return PVLOG_CHECK_ERROR_SOURCE;
  }
  if(check_pid(kvs)<0)
  {
    MON_ADD(LOG_CHECK_ERROR_NO_PID, 1);
    return PVLOG_CHECK_NO_PID;
  }
  check_youtu(kvs);
  check_iqiyi(kvs);
  check_toutiao(kvs);
}

int PvLogRules::check_pid(boost::unordered_map<string,string> & kvs)
{
  if(kvs.count("pid")==0)
    return -1;
  string &source=kvs["source"];
  boost::unordered_map<string,MinutePidNum>::iterator iter=minute_pid_num_map_.find(source);
  if(iter==minute_pid_num_map_.end())
    return 0;
  string &pid=kvs["pid"];
  int pid_num=iter->second.set(pid);
  if(pid_num>FLAGS_max_minute_pids)
  {
    MON_ADD(LOG_CHECK_STAT_TOO_MANY_PID, 1);
    LOG_ERROR("source %s has too many pid,%s",source.c_str(),pid.c_str());
  }
  return 0;
}

int PvLogRules::check_youtu(boost::unordered_map<string,string> & kvs)
{
  string &source=kvs["source"];
  if(source.compare("2")!=0)
    return 0;
  bool snd_stat=false;
  string &trace_id=kvs["trace_id"];
  if(kvs.count("title")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no title,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(kvs.count("keywords")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no keywords,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(kvs.count("vid")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no vid,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(kvs.count("show_id")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no show_id,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(kvs.count("cnl")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no cnl,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(kvs.count("cnl2")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no cnl2,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(kvs.count("video_owner")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no video_owner,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(snd_stat)
  {
    MON_ADD(LOG_CHECK_STAT_YOUTU_LESS_PARAM, 1);
  }
  return 0;
}

int PvLogRules::check_iqiyi(boost::unordered_map<string,string> & kvs)
{
  string &source=kvs["source"];
  if(source.compare("6")!=0)
    return 0;
  bool snd_stat=false;
  string &trace_id=kvs["trace_id"];
  if(kvs.count("keywords")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no keywords,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(kvs.count("show_id")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no show_id,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(kvs.count("cnl")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no cnl,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  
  if(snd_stat)
  {
    MON_ADD(LOG_CHECK_STAT_IQIYI_LESS_PARAM, 1);
  }
  return 0;
}

int PvLogRules::check_toutiao(boost::unordered_map<string,string> & kvs)
{
  string &source=kvs["source"];
  if(source.compare("7")!=0)
    return 0;
  bool snd_stat=false;
  string &trace_id=kvs["trace_id"];
  if(kvs.count("cnl")==0)
  {
    snd_stat=true;
    LOG_DEBUG("source %s has no cnl,trace_id=%s",source.c_str(),trace_id.c_str());
  }
  if(snd_stat)
  {
    MON_ADD(LOG_CHECK_STAT_TOUTIAO_LESS_PARAM, 1);
  }
  return 0;
}

}
}