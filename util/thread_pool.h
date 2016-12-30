#pragma once

#include "boost/threadpool.hpp"
#include "mutex.h"
#include "boost/serialization/singleton.hpp"
#include "closure.h"

namespace poseidon
{
namespace util
{
	class ThreadPool:public boost::serialization::singleton<ThreadPool>
	{
		public:
			ThreadPool();
			virtual ~ThreadPool();
			bool add_task(boost::threadpool::task_func const & task);
      bool add_task(Closure * task);
			void init(int thread_num);
			void init();
			int get_thread_num()
			{
				return _thread_num;
			}
		private:
			boost::threadpool::pool *thread_pool;
			bool is_init;
			Mutex init_mutex;
			int _thread_num;
	};
}
}
