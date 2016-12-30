/**
 **/

#ifndef  _YUNOS_GAME_MAP_ 
#define  _YUNOS_GAME_MAP_


#include <boost/serialization/singleton.hpp>
#include <map>
#include <set>
#include <vector>
#include <iostream>
#include <boost/atomic.hpp>
#include "mutex.h"
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace control
{
  class YunosGameMap : public boost::serialization::singleton<YunosGameMap>
  {
    public:
      bool init(const std::string &path);
      bool is_mapped(const std::string  &yunos_category,const common::Creative & creative);
      
    private:
      bool _is_init;
      util::Mutex _init_mutex;
      std::map<std::string,std::set<int> > _yunos_game_map;
  };
}

}

#endif