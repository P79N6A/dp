#pragma once

#include "injector_inc.h"

namespace poseidon
{
  class DeviceTask:public boost::serialization::singleton<DeviceTask>
  {
    public:
      DeviceTask()
      {
        _running=false;
      }
      void process();
      
    protected:
      bool _running;
      util::Mutex _run_mutex;
      MYSQL _mysql;
    protected:
      void wrire_redis(dmp::DmpDevicePriceDatas & datas);
  };
}