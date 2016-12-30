#include "mutex.h"

namespace poseidon
{
namespace util
{
	Mutex::Mutex()
	{
		pthread_rwlockattr_init(&_rwaddr);
		pthread_rwlock_init(&_rwlock, &_rwaddr);
		is_locked = false;
	}

	Mutex::~Mutex()
	{
		if(is_locked)
		{
			unlock();
		}
		pthread_rwlock_destroy(&_rwlock);
		pthread_rwlockattr_destroy(&_rwaddr);
	}

	void Mutex::rlock()
	{
		is_locked = true;
		pthread_rwlock_rdlock(&_rwlock);
	}


	void Mutex::wlock()
	{
		is_locked = true;
		pthread_rwlock_wrlock(&_rwlock);
	}

	bool Mutex::rtry()
	{
		if(pthread_rwlock_tryrdlock(&_rwlock)==0)
		{
			is_locked = true;
			return true;
		}
		return false;
	}


	bool Mutex::wtry()
	{
		if(pthread_rwlock_trywrlock(&_rwlock)==0)
		{
			is_locked = true;
			return true;
		}
		return false;
	}



	void Mutex::unlock()
	{
		pthread_rwlock_unlock(&_rwlock);
		is_locked = false;
	}
}
}