/** 
 * Copyright (c) 2014 Taobao.com
 * All rights reserved.
 * 文件名称：OpenDspSingleton.h
 * 摘要：
 * 作者：jimmy.gj<jimmy.gj@taobao.com>
 * 日期：2014.09.23
 * 修改摘要：
 * 修改者：
 * 修改日期：
 */
#ifndef OPEN_DSP_SINGLETON_H
#define OPEN_DSP_SINGLETON_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

template <class T> class OpenDspSingleton
{
	public:
		static T* getInstance();
		static void destroy();

	protected:
		OpenDspSingleton();
		~OpenDspSingleton();
    
	private:
		static T *_instance;
		static pthread_mutex_t _mutex;
};

template <class T> T* OpenDspSingleton<T>::_instance = NULL;
template <class T> pthread_mutex_t OpenDspSingleton<T>::_mutex = PTHREAD_MUTEX_INITIALIZER;

template <class T> T* OpenDspSingleton<T>::getInstance()
{
	if(_instance == NULL)
	{
		pthread_mutex_lock(&_mutex);
		if(_instance == NULL)
		{
			_instance = new T();
		}
		pthread_mutex_unlock(&_mutex);
	}
	return _instance;
}

template <class T> void OpenDspSingleton<T>::destroy()
{
	if(_instance)
	{
        pthread_mutex_lock(&_mutex);
        if(_instance)
        {
            delete _instance;
            _instance = NULL;
        }
        pthread_mutex_unlock(&_mutex);
	}
}

#endif

