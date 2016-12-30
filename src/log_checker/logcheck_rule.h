#pragma once

#include "logcheck_inc.h"

namespace poseidon {
namespace log_check {
  
class CheckRule
{
  public:
    virtual int check(boost::unordered_map<string,string> & kvs)=0;
};

}
}