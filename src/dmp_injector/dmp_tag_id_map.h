#pragma once

#include "injector_inc.h"

namespace poseidon
{
  
struct TagIdInfo
{
  string tag_id;
  uint32_t tag_no;
  int value_type;
};

class TagIdMap:public boost::serialization::singleton<TagIdMap>
{
  public:
    bool init();
    void get_all(vector<TagIdInfo> & tag_infos);
  
  protected:
    boost::unordered_map<string,TagIdInfo> _tag_id_map;
};

}
