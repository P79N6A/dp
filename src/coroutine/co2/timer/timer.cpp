/*
 * timer.cpp
 * Created on: 2016-11-29
 */

#include "co2/timer/timer.h"

#include "co2/co2.h"

namespace co2 {
namespace timer {

void *timer_thread(void *);

Timer::Timer(const std::function<void(void *)> &timer_cb,
    int timeout, void *arg) :
    timer_cb_(timer_cb), timeout_(timeout), arg_(arg), 
    running_(false), timer_th_(NULL)
{

}

Timer::~Timer()
{
    Stop();
}

Timer *Timer::Create(const std::function<void(void *)> &timer_cb,
    int timeout, void *arg)
{
    /* timer use microseconds as timeout unit. */
    Timer *timer = new Timer(timer_cb, timeout * 1000, arg);
    return timer;
}

int Timer::Start()
{
    if (!running_) {
        timer_th_ = st_thread_create(timer_thread, this, 0, 32768);
        if (timer_th_) {
            running_ = true;
            return kOK;
        } else {
            return kMemoryErr;
        }
    } else {
        return kOK;
    }
}

void * timer_thread(void * arg)
{
    co2::Log(INFO, "timer start\n");
    co2::timer::Timer *timer = (Timer *)arg;
    while (!timer->IsStop()) {
        st_usleep((st_utime_t)(timer->GetTimeout()));
        timer->GetCallback()(timer->GetArg());
    }
    return (void *)0;
}

int Timer::Stop(void)
{
    running_ = false;
    void *ret = NULL;

    if (timer_th_) {
        st_thread_join(timer_th_, &ret);
    }
    return ret == NULL ? kOK : kUnknownErr;
}

}  // timer
}  // namespace co2

