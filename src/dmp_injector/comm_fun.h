#pragma once

#include "injector_inc.h"

#define BULKET_INDEX_LEN 2

namespace poseidon
{
  static int get_all_filenames(const string & path,vector<string> & file_names)
  {
    DIR *dp;
    struct dirent *dirp;
    if ((dp = opendir(path.c_str())) == NULL)
        return -1;
    while ((dirp = readdir(dp)) != NULL)
    {
      if(strcmp(dirp->d_name,".")==0 || strcmp(dirp->d_name,"..")==0)
        continue;
      file_names.push_back(dirp->d_name);
    }
    closedir(dp);
    return 0;
  }
  static bool uid2num(const string &uid,uint64_t &num)
  {
    for(int i=0;i<uid.length();i++)
    {
      if(i==0)
      {
        if(uid.at(i)=='0')
          return false;
      }
      if(uid.at(i)<'0' || uid.at(i)>'9')
      {
        return false;
      }
    }
    num=(uint64_t)strtoul(uid.c_str(),NULL,10);
    return true;
  }
  static string get_bucket_key(const string & uid)
  {
    string bucket_key;
    if(uid.length()>BULKET_INDEX_LEN)
      bucket_key=uid.substr(0,uid.length()-BULKET_INDEX_LEN);
    else
      bucket_key=uid;
    return bucket_key;
  }
  
  static string get_bucket_index(const string & uid)
  {
    string bucket_index;
    if(uid.length()>BULKET_INDEX_LEN)
      bucket_index=uid.substr(uid.length()-BULKET_INDEX_LEN,BULKET_INDEX_LEN);
    else
      bucket_index=uid;
    return bucket_index;
  }
  
}