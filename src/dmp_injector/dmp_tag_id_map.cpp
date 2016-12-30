#include "dmp_tag_id_map.h"

DEFINE_string(dmp_tag_id_dict, "../data/dmp.tagid.dict", "dmp_tag_id_dict path");
namespace poseidon
{
  bool TagIdMap::init()
  {
    char line[1024]={0};
    ifstream fin(FLAGS_dmp_tag_id_dict.c_str());
    if(fin.is_open())
    {
      while(fin.getline(line,sizeof(line)-1))
      {
        if(strncmp(line,"#",1)==0)
          continue;
        vector<string> columns;
        boost::split(columns, line, boost::is_any_of(",;`"));  
        TagIdInfo info;
        if(columns.size()==3)
        {
          info.tag_id=columns[0];
          info.tag_no=(uint32_t)strtoul(columns[1].c_str(),NULL,10);
          info.value_type=strtoul(columns[2].c_str(),NULL,10);
          _tag_id_map[info.tag_id]=info;
        }
        memset(line,0,sizeof(line));
      }
    }
    else
    {
      cout<<"dmp_tag_id_dict path error,and exit..."<<endl;
      return false;
    }
    return true;
  }
  
  void TagIdMap::get_all(vector<TagIdInfo> & tag_infos)
  {
    boost::unordered_map<string,TagIdInfo>::iterator iter;
    for(iter=_tag_id_map.begin();iter!=_tag_id_map.end();iter++)
    {
      tag_infos.push_back(iter->second);
    }
  }
}