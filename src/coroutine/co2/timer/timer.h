/*
 * timer.h
 * Created on: 2016-11-29
 */

#ifndef CO2_TIMER_TIMER_H_
#define CO2_TIMER_TIMER_H_

#include <functional>
#include <st/st.h>

namespace co2 {
namespace timer {

class Timer {
public:
    ~Timer(void);

    /* create timer, specify a callback function and
     * timeout in milliseconds */
    static Timer *Create(const std::function<void(void *)> &timer_cb,
        int timeout, void *arg = NULL);

    template<class T>
    static Timer *Create(void (T::*M)(void *), T &obj, int timeout, void *arg = NULL)
    {
        return Create(std::bind(M, &obj, std::placeholders::_1),
            timeout, arg);
    }

    /* Start timer periodically, note that a timer is indeed a coroutine,
     * so this is a little bit heavy than the timer in libevent */
    int Start(void);

    /* Stop timer */
    int Stop(void);

    /* the following function should not be called by user. */
    bool IsStop(void) { return !running_; }
    int GetTimeout(void) { return timeout_; }
    void *GetArg(void) { return arg_ ; }
    std::function<void(void *)> &GetCallback(void) { return timer_cb_; }

protected:
    Timer(const Timer&) = delete;
    Timer &operator=(const Timer &) = delete;

    /* create a timer, interval is microseconds of timeout value */
    Timer(const std::function<void(void *)> &timer_cb,
        int timeout, void *arg = 0);

protected:
    std::function<void(void *)> timer_cb_;
    int timeout_;
    void *arg_;
    bool running_;
    st_thread_t timer_th_;
};

}}

#endif /* CO2_TIMER_TIMER_H_ */
