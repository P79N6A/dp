/*
 * mutex.h
 * Created on: 2016-12-13
 */

#ifndef CO2_SYNC_MUTEX_H_
#define CO2_SYNC_MUTEX_H_

#include <st/st.h>

namespace co2 {
namespace sync {

class Mutex {
public:
    Mutex(void);
    ~Mutex(void);
    int Init(void);
    int Lock(void);
    int TryLock(void);
    int Unlock(void);

protected:
    st_mutex_t mutex_;
};


}  // namespace sync
}  // namespace co2



#endif /* SRC_COROUTINE_CO2_SYNC_MUTEX_H_ */
