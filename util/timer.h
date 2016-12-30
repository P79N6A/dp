#pragma once

namespace poseidon
{
namespace util  
{
	class Timer 
	{
		public:
			static uint64_t get_timestamp()
			{
				struct timeval now_time;
				gettimeofday( &now_time, NULL );
				uint64_t time_stamp=((uint64_t)now_time.tv_sec)*1000*1000+now_time.tv_usec;
				return time_stamp;
			}
			void start() 
			{
				start_time_ = get_timestamp();
				last_stop_ = start_time_;
			}

			uint64_t stop() const 
			{
				uint64_t stop_time = get_timestamp();
				return stop_time - start_time_;
			}

			uint64_t interval()
			{
				uint64_t stop_time = get_timestamp();
				uint64_t delta_time = stop_time - last_stop_;
				last_stop_ = stop_time;
				return delta_time;
			}

		private:
			uint64_t start_time_;
			uint64_t last_stop_;
	};
}
}