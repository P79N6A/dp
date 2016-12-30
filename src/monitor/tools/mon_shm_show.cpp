/**
 **/

#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include "../common/define.h"

#include "shm.h"

using namespace poseidon::monitor;

int main(int argc, char * argv[])
{
    poseidon::monitor::MonitorLayOut * layout_=(poseidon::monitor::MonitorLayOut *)poseidon::util::shm::ShmAttach(MONITOR_KEY, sizeof(MonitorLayOut), 0444);
    std::cout<<"head:"<<std::endl;
    std::cout<<"\t"<<"max_attr_id:"<<layout_->head_.max_attr_id_<<std::endl;
    std::cout<<"body:"<<std::endl;
    for(int i=0; i<=layout_->head_.max_attr_id_; i++)
    {
        if(layout_->body_.attr_[i].usedflag_ == USED_FLAG )
        {
            std::cout<<"\tattr_id:"<<i<<std::endl;
            int index=layout_->body_.attr_[i].index_;
            std::cout<<"\t\tindex:"<<index<<std::endl;
            std::cout<<"\t\tmin:"<<layout_->body_.attr_[i].data_[index].min_<<", value:"<<layout_->body_.attr_[i].data_[index].value_<<std::endl;
        }
    }
    return 0;

}
