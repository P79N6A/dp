/*
 * cond.cpp
 * Created on: 2016-12-13
 */

#include "co2/sync/cond.h"
#include "co2/co2.h"

namespace co2 {
namespace sync {

using namespace co2;

Cond::Cond(void) : cond_(NULL) {}

Cond::~Cond(void)
{
    if (cond_) {
        st_cond_destroy(cond_);
    }
}

int Cond::Init(void)
{
    if (!cond_) {
        cond_ = st_cond_new();
    }
    return cond_ ? kOK : kMemoryErr;
}

int Cond::Signal(void)
{
    return st_cond_signal(cond_);
}

int Cond::Broadcast(void)
{
    return st_cond_broadcast(cond_);
}

int Cond::Wait(void)
{
    return st_cond_timedwait(cond_, ST_UTIME_NO_TIMEOUT);
}

int Cond::TimeWait(int milliseconds)
{
    return st_cond_timedwait(cond_, milliseconds * 1000);
}

}  // namespace sync
}  // namespace co2
