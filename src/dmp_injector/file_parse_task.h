#pragma once

#include "injector_inc.h"

namespace poseidon
{
  enum DmpTagValueType
  {
    kValueSingleType=1,
    kValueJsonType=2,
    kValueArrayType=3,
  };

  class FileParseTask
  {
    public:
      FileParseTask(uint16_t tag_no,int value_type,string file_path)
      {
        _tag_no=tag_no;
        _value_type=value_type;
        _file_path=file_path;

      }
      bool parse();
      
    protected:
      uint16_t _tag_no;
      int _value_type;
      string _file_path;
    
    protected:
      void parseLine(const vector<string> &columns);
      void ParseJsonLine(const vector<string> &columns);
      void parseArrayLine(const vector<string> &columns);
  };
}