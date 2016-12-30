#pragma once

#include "event.h"

namespace poseidon
{
namespace util  
{
  class TimeoutEvent
  {
    public:
      TimeoutEvent(struct event_base * base,int sec,int usec,event_callback_fn cb,void *arg)
      {
        _timeout_event=NULL;
        _base=base;
        _tv.tv_sec=sec;
        _tv.tv_usec=usec;
        _cb=cb;
        _arg=arg;
      }
      virtual ~TimeoutEvent()
      {
        if(_timeout_event!=NULL)
        {
          evtimer_del(_timeout_event);
          event_free(_timeout_event);
        }
      }
      void active()
      {
        _timeout_event = evtimer_new(_base, _cb, (void *)_arg);
        evtimer_add(_timeout_event, &_tv); 
      }
      
    protected:
      struct event_base * _base;
      struct timeval _tv;
      event_callback_fn _cb;
      void * _arg;
      struct event * _timeout_event;
  };
}
}