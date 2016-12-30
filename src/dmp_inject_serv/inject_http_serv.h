#pragma once
#include "inject_inc.h"
#include "inject_worker.h"

namespace poseidon
{
namespace inject
{

	class HttpServer:public boost::serialization::singleton<HttpServer>
	{

		public:
			HttpServer();
			virtual ~HttpServer();
			void startup();
		protected:
			uint32_t _port;
      uint32_t _worker_num;
			evutil_socket_t _listen_fd;
			uint32_t _http_time_out;
      vector<HttpWorker *> _workers;
			bool running;
	};

}
}