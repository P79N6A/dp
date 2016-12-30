/*
 * cond.h
 * Created on: 2016-12-13
 */

#ifndef CO2_SYNC_COND_H_
#define CO2_SYNC_COND_H_

#include <st/st.h>

namespace co2 {
namespace sync {

class Cond {
public:
    Cond(void);
    ~Cond(void);
    int Init(void);
    int Wait(void);
    int TimeWait(int milliseconds);
    int Signal(void);
    int Broadcast(void);

protected:
    st_cond_t cond_;
};


}  // namespace sync
}  // namespace co2



#endif /* CO2_SYNC_COND_H_ */
