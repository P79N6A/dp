/**
 **/


#include "json/json.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <cstring>

using namespace std;

int main(int argc, char * argv[])
{
    int rt=0;
    do{
        ifstream ifs;
        ifs.open("test.json");
        assert(ifs.is_open());
        char buf[1024];
        while(1)
        {
            memset(buf, 0x00, 1024);
            ifs.getline(buf, 1024);
            Json::Reader reader;
            Json::Value root;
            if(!reader.parse(buf, root, false))
            {
                return -1;
            }
            std::string targets=root["targeted_package"].asString();
            std::cout<<targets<<std::endl;
            Json::Value tarroot;
            if(!reader.parse(targets, tarroot, false))
            {
                printf("error\n");
                return -1;
            }
#if 0
            Json::Value::Members mem=tarroot.getMemberNames();
            Json::Value::Members::iterator it;
            for(it=mem.begin(); it != mem.end(); it++)
            {
                std::cout<<*it<<std::endl;
                switch(tarroot[*it].type())
                {
                    case Json::arrayValue:
                }
            }

            Json::Value tarroot=root["targeted_package"];
#endif
            std::cout<<tarroot["1001"][0].asInt()<<std::endl;
            std::cout<<tarroot["1002"][0].asInt()<<std::endl;
            std::cout<<tarroot["1003"][0].asInt()<<std::endl;

#if 0
            std::string name;
            if(root["name"].isNull())
            {
                name="zhang3";
            }else
            {
                name=root["name"].asString();
            }
            root.begin
            int age = root["age"].asInt();
            std::cout<<name<<std::endl;
            std::cout<<age<<std::endl;
#endif
        }
    }while(0);
    return rt;
}
