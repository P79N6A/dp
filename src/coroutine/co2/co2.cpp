/*
 * co2.cpp
 * Created on: 2016-12-08
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include "co2.h"

namespace co2 {

/* global variables */
int log_level = INFO;

st_netfd_t state_thread_fds[65536];

int Log(int l, const char *fmt, ...)
{
#ifdef ST_DEBUG
    if (l < log_level)
        return -1;

    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);

    if (l == FATAL)
        abort();
#endif
    return 0;
}

/* Call this function to Init Coroutine Library.
 * Thie function should be called after forking.
 */
int Init(void)
{
    /* set platform dependent event polling first. */
    static bool state_thread_init;

    if (!state_thread_init) {
        st_set_eventsys(ST_EVENTSYS_ALT);
        if (st_init() != 0) {
            Log(FATAL, "state thread init failed!\n");
            return -1;
        }

        memset(state_thread_fds, 0, sizeof(state_thread_fds));
        state_thread_init = true;
    }
    return 0;
}


}  // namespace co2
