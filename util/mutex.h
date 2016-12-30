/**
 **/

#pragma once
#include <pthread.h>

namespace poseidon
{
namespace util
{
	class Mutex
	{
		protected:
			pthread_rwlockattr_t _rwaddr;
			pthread_rwlock_t _rwlock;
			bool is_locked;
			
		public:
			Mutex();
			~Mutex();
			void wlock();
			void rlock();
			void unlock();
			bool rtry();
			bool wtry();
	};
  class ScopeRMutex
  {
    public:
      ScopeRMutex(Mutex * mutex)
      {
        _mutex=mutex;
        _mutex->rlock();
      }
      virtual ~ScopeRMutex()
      {
        _mutex->unlock();
      }
    
    private:
      Mutex * _mutex;
  };
  
  class ScopeWMutex
  {
    public:
      ScopeWMutex(Mutex * mutex)
      {
        _mutex=mutex;
        _mutex->wlock();
      }
      virtual ~ScopeWMutex()
      {
        _mutex->unlock();
      }
    
    private:
      Mutex * _mutex;
  };
}
}
