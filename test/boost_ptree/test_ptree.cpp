#include <boost/property_tree/ptree.hpp>    
#include <boost/property_tree/ini_parser.hpp>    
#include <string>    
#include <iostream>    
using namespace std;   
  
int main()  
{  
    boost::property_tree::ptree m_pt;  
    string ini_file = "test.ini";  
    boost::property_tree::ini_parser::read_ini(ini_file, m_pt);    
    string path  = m_pt.get<string>("abc.LogPath","");  
    int maxcount  = m_pt.get<int>("abc.LogMaxCount", 1);  
    std::cout << path << "::"  << maxcount<<std::endl;  
  
    return 0;  
}  
