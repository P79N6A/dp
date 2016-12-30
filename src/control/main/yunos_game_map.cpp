#include "yunos_game_map.h"
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <boost/algorithm/string.hpp> 
#include "util/log.h"
#include "json/json.h"

namespace poseidon
{
namespace control
{
  bool YunosGameMap::init(const std::string &path)
  {
    if(_is_init)
      return false;
    util::ScopeWMutex swMutex(&_init_mutex);
    if(_is_init)
      return false;

    char line[1024]={0};
    std::ifstream fin(path.c_str());
    if(fin.is_open())
    {
      while(fin.getline(line,sizeof(line)-1))
      {
        std::vector<std::string> columns;
        boost::split(columns, line, boost::is_any_of("\t,;"));  
        if(columns.size()>=2)
        {
          std::string yunos_category=columns[2];;
          int ali_category=strtoul(columns[1].c_str(),NULL,10);
          std::map<std::string,std::set<int> >::iterator iter=_yunos_game_map.find(yunos_category);
          if(iter!=_yunos_game_map.end())
          {
            iter->second.insert(ali_category);
          }
          else
          {
            std::set<int> tmp;
            tmp.insert(ali_category);
            _yunos_game_map[yunos_category]=tmp;
          }
        }
        memset(line,0,sizeof(line));
      }
    }
    else
      return false;
    if(_yunos_game_map.size()==0)
    {
      LOG_ERROR("_yunos_game_map load error,size is 0");
    }
    _is_init=true;
    return true;
  }
  
  bool YunosGameMap::is_mapped(const std::string & yunos_category,const common::Creative & creative)
  {
    if(!_is_init)
      return false;
    
    //为适配yunos广告
    if(yunos_category.compare("单机")==0 || yunos_category.compare("网游")==0)
    {
      if(creative.has_specific_data())
      {
        Json::Reader reader; 
        Json::Value special_data;
        if(reader.parse(creative.specific_data(),special_data))
        {
          Json::Value type_id=special_data["typeid"];
          if(type_id.isString())
          {
            if(type_id.asString().compare("15")==0 && yunos_category.compare("网游")==0)
              return true;
            if( (type_id.asString().compare("17")==0 || type_id.asString().compare("18")==0) && yunos_category.compare("单机")==0)
              return true;
          }
          else if(type_id.isInt())
          {
            if(type_id.asInt()==15 && yunos_category.compare("网游")==0)
              return true;
            if( (type_id.asInt()==17 || type_id.asInt()==18) && yunos_category.compare("单机")==0)
              return true;
          }
        }
      }
      return false;
    }
    
    std::map<std::string,std::set<int> >::iterator iter=_yunos_game_map.find(yunos_category);
    if(iter==_yunos_game_map.end())
    {
      return false;
    }
    else
    {
      for(int i=0;i<creative.creative_category_size();i++)
      {
        if(iter->second.count(creative.creative_category(i))>0)
        {
          return true;
        }
      }
    }
    return false;
  }
}

}