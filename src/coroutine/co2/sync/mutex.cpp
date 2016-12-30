/*
 * mutex.cpp
 * Created on: 2016-12-13
 */

#include "co2/sync/mutex.h"
#include "co2/co2.h"

namespace co2 {
namespace sync {

Mutex::Mutex(void) : mutex_(NULL)
{

}

Mutex::~Mutex(void)
{
    if (mutex_) {
        st_mutex_destroy(mutex_);
    }
}

int Mutex::Init(void)
{
    if (!mutex_) {
        mutex_ = st_mutex_new();
    }
    return mutex_ ? kOK : kMemoryErr;
}

int Mutex::Lock(void)
{
    return st_mutex_lock(mutex_);
}

int Mutex::TryLock(void)
{
    return st_mutex_trylock(mutex_);
}

int Mutex::Unlock(void)
{
    return st_mutex_unlock(mutex_);
}

}  // namespace sync
}  // namespace co2


