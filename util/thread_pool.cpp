#include "thread_pool.h"

namespace poseidon
{
namespace util
{
	ThreadPool::ThreadPool()
	{
		thread_pool = NULL;
		is_init = false;
		_thread_num=0;
	}
	
	ThreadPool::~ThreadPool()
	{
		if(thread_pool != NULL)
			delete thread_pool;
	}
	
	void ThreadPool::init(int thread_num)
	{
		if(is_init)
			return;
		init_mutex.wlock();
		if(is_init)
		{
			init_mutex.unlock();
			return;
		}
		_thread_num=thread_num;
		thread_pool=new boost::threadpool::pool(thread_num);
		is_init = true;
		init_mutex.unlock();
	}
	
	void ThreadPool::init()
	{
		if(is_init)
			return;
		int cpu_num=(int)sysconf(_SC_NPROCESSORS_ONLN);
		init(cpu_num);
	}
	
	
	bool ThreadPool::add_task(boost::threadpool::task_func const & task)
	{
		init();
		return thread_pool->schedule(task);
	}
  
  void task_closure_run(Closure * task)
  {
    task->Run();
  }
  
  bool ThreadPool::add_task(Closure * task)
  {
    init();
		return thread_pool->schedule(boost::bind(task_closure_run,task));
  }
}
}
