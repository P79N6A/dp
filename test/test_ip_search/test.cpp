#include "util/ip_search.h"

using namespace std;

int main(int argc, char * argv [])
{
  std::string ip_file_path="/home/wangyj/dsp/Poseidon/envs/poseidon/data/ip_2016-08-24.txt";
  poseidon::util::IpSearch::get_mutable_instance().init(ip_file_path,2048);
  sleep(3);
  cout<<"start time : "<<time(time_t(NULL))<<endl;
  for(uint32_t search_ip=16777470;search_ip<401236311;search_ip++)
  {
    uint32_t city_code=0;
    uint32_t carrier_code=0;
    int search_ret=poseidon::util::IpSearch::get_mutable_instance().search(search_ip,city_code,carrier_code);
    cout<<"search_ret="<<search_ret<<
          ";search_ip="<<search_ip<<
          ";city_code="<<city_code<<
          ";carrier_code="<<carrier_code<<endl;
  }
  cout<<"end time : "<<time(time_t(NULL))<<endl;
  return 0;
}