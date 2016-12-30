#include "file_parse_task.h"
#include "protocol/src/poseidon_dmp_tag_data.pb.h"
#include "task_scheduler.h"
#include "local_redis_map.h"


namespace poseidon
{
  class ScopeReply
  {
    public:
      ScopeReply(redisReply * r)
      {
        reply=r;
      }
      virtual ~ScopeReply()
      {
        freeReplyObject(reply);
      }
    private:
      redisReply *reply;
  };

  bool FileParseTask::parse()
  {
    char line[1024*256]={0};
    ifstream fin(_file_path.c_str());
    if(fin.is_open())
    {
      cout<<"start parse"<<_file_path<<",tag_no is "<<_tag_no<<endl;
      uint64_t line_num=0;
      while(fin.getline(line,sizeof(line)-1))
      {
        line_num++;
        vector<string> columns;
        boost::split(columns, line, boost::is_any_of("`"));  
        if(columns.size()>=2)
        {
          if(_value_type==kValueSingleType)
            parseLine(columns);
          else if(_value_type==kValueJsonType)
            ParseJsonLine(columns);
          else if(_value_type==kValueArrayType)
            parseArrayLine(columns);
          else
          {
            cout<<"value type error["<<_value_type<<"]"<<endl;
            return false;
          }
        }
        else
        {
          cout<<"line parse error["<<line<<"]"<<endl;
        }
        memset(line,0,sizeof(line));
      }
      cout<<"end parse"<<_file_path<<",line_num : "<<line_num<<endl;
    }
    else
    {
      cout<<"file "<<_file_path<<" can not found"<<endl;
      return false;
    }
    return true;
  }
  void FileParseTask::parseLine(const vector<string> &columns)
  {
    uint32_t value=strtoul(columns[1].c_str(),NULL,10);
    while(!LocalRedisMap::get_mutable_instance().insert(columns[0],_tag_no,value))
    {
      ::sleep(1);
    }
  }
  
  void FileParseTask::parseArrayLine(const vector<string> &columns)
  {
    string json_str=columns[1];
    Json::Reader reader; 
    Json::Value root;
    vector<uint32_t> tag_values;
    if(reader.parse(json_str,root))
    {
      if(root.isArray())
      {
        for(int i=0;i<root.size();i++)
        {
          if(root[i].isInt())
            tag_values.push_back(root[i].asInt());
        }
        if(tag_values.size()>0)
        {
          while(!LocalRedisMap::get_mutable_instance().insert(columns[0],_tag_no,tag_values))
          {
            ::sleep(1);
          }
        }
      }
    }
  }
  
  void FileParseTask::ParseJsonLine(const vector<string> &columns)
  {
    string json_str=columns[1];
    Json::Reader reader; 
    Json::Value root;
    vector<uint32_t> tag_values;
    if(reader.parse(json_str,root))
    {
      Json::Value::Members::iterator iter;
      Json::Value::Members members = root.getMemberNames();
      for(iter = members.begin(); iter != members.end(); iter++)
      {
        uint32_t value=strtoul((*iter).c_str(),NULL,10);
        tag_values.push_back(value);
      }
      if(tag_values.size()>0)
      {
        while(!LocalRedisMap::get_mutable_instance().insert(columns[0],_tag_no,tag_values))
        {
          ::sleep(1);
        }
      }
    }
    else
    {
      cout<<"json parse error["<<json_str<<"]"<<endl;
    }
  }
  
}