#include "logcheck_rule.h"

namespace poseidon {
namespace log_check {

enum PvLogErrorCode
{
  PVLOG_CHECK_NO_ERROR=0,
  PVLOG_CHECK_NO_SOURCE=-1,
  PVLOG_CHECK_ERROR_SOURCE=-2,
  PVLOG_CHECK_NO_PID=-3,
  PVLOG_CHECK_NO_KEYWORDS=-4,
};

class MinutePidNum
{
  public:
    MinutePidNum()
    {
      minute_=0;
    }
    int set(const string & pid);
  protected:
    int minute_;
    boost::unordered_set<string> pids_;
};

class PvLogRules:public CheckRule
{
  public:
    PvLogRules()
    {
      is_init_=false;
    }
    void init();
    virtual int check(boost::unordered_map<string,string> & kvs);
    
  protected:
    boost::unordered_set<string> source_set_;
    boost::unordered_map<string,MinutePidNum> minute_pid_num_map_;
  protected:
    bool is_init_;
    int check_pid(boost::unordered_map<string,string> & kvs);
    int check_youtu(boost::unordered_map<string,string> & kvs);
    int check_iqiyi(boost::unordered_map<string,string> & kvs);
    int check_toutiao(boost::unordered_map<string,string> & kvs);
};
 
}
}