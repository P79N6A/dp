#pragma once
#include "inject_inc.h"
#include "inject_context.h"

namespace poseidon
{
namespace inject
{
  
  class HttpWorker
  {
		public:
      HttpWorker();
      virtual ~HttpWorker();
			void startup(evutil_socket_t fd,int time_out);
			virtual void recv_request(struct evhttp_request *req);
		protected:
      evutil_socket_t _fd;
      struct evhttp *_httpd;
			struct event_base *_base;
			uint32_t _http_time_out;
      util::RedisClient _redis_client;
      uint64_t _serial_num;
      boost::unordered_map<uint64_t,boost::shared_ptr<InjectContext> > _context_map;
		
		protected:
			void run();
      void schedule(boost::shared_ptr<InjectContext> context);
      void process(boost::shared_ptr<InjectContext> context);
      void get_process(boost::shared_ptr<InjectContext> context);
      void set_process(boost::shared_ptr<InjectContext> context);
      void del_process(boost::shared_ptr<InjectContext> context);
			void http_done(boost::shared_ptr<InjectContext> context);
      void get_user_tags(boost::shared_ptr<InjectContext> context);
      void on_got_user_tags(uint64_t context_id,redisReply *reply);
      void set_user_tag(boost::shared_ptr<InjectContext> context);
      void on_set_user_tag(uint64_t context_id,redisReply *reply);
      void del_user_tags(boost::shared_ptr<InjectContext> context);
      void on_del_user_tags(uint64_t context_id,redisReply *reply);
  };
  typedef boost::unordered_map<uint64_t,boost::shared_ptr<InjectContext> > ::iterator ContextIter;
}
}