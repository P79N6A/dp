/**
 **/

#include "monitor_api.h"
#include "util/shm.h"
#include <unistd.h>
#include <time.h>

namespace poseidon
{
namespace monitor
{

/**
 * @brief       初始化
 **/
int Api::init()
{
    int rt=0;
    do{
        layout_=(MonitorLayOut *)util::shm::ShmAttach(MONITOR_KEY, sizeof(MonitorLayOut), 0666);
        if(layout_ == NULL)
        {
            rt=-1;
            break;
        }
        init_=true;
    }while(0);
    return rt;
}

/**
 * @brief       属性上报add
 **/
void Api::mon_add(int attr_id, int val)
{
    if(attr_id < 0 || attr_id > MAX_ATTR_ID || val < 0 )
    {
        return;
    }
    if(!init_)
    {
        if(init() != 0)
        {
            return;
        }
    }


    Attr & attr=layout_->body_.attr_[attr_id];
    if(attr.usedflag_ != USED_FLAG)
    {
        attr.usedflag_=USED_FLAG;
        attr.index_=0;
        attr.data_[0].min_=time(NULL)/60;
        attr.data_[0].value_=0;
#if 0
        bool flag;  
        do{
            int old_max_attr_id=layout_->head_.MonitorHead::max_attr_id_;
            if(attr_id > old_max_attr_id)
            {
                flag=__sync_bool_compare_and_swap(&(layout_->head_.MonitorHead::max_attr_id_), old_max_attr_id, attr_id);
            }else
            {
                break;
            }
        }while(!flag);
#endif
    }
    /*不做CAS了，每次都判断下*/
    if(attr_id > layout_->head_.MonitorHead::max_attr_id_)
    {
        layout_->head_.MonitorHead::max_attr_id_=attr_id;
    }

//    update_index(attr);
    MinData & mindata=attr.data_[attr.index_];
    __sync_fetch_and_add(&mindata.value_, (int64_t)val);
}

/**
 * @brief       属性上报set
 **/
void Api::mon_set(int attr_id, int val)
{
    if(attr_id < 0 || attr_id > MAX_ATTR_ID || val < 0 )
    {
        return;
    }
    if(!init_)
    {
        if(init() != 0)
        {
            return;
        }
    }


    Attr & attr=layout_->body_.attr_[attr_id];
    if(attr.usedflag_ != USED_FLAG)
    {
        attr.usedflag_=USED_FLAG;
        attr.index_=0;
        attr.data_[0].min_=time(NULL)/60;
        attr.data_[0].value_=0;
#if 0
        bool flag;
        do{
            int old_max_attr_id=layout_->head_.MonitorHead::max_attr_id_;
            if(attr_id > old_max_attr_id)
            {
                flag=__sync_bool_compare_and_swap(&(layout_->head_.MonitorHead::max_attr_id_), old_max_attr_id, attr_id);
            }else
            {
                break;
            }
        }while(!flag);
#endif
    }
    /*不做CAS了，每次都判断下*/
    if(attr_id > layout_->head_.MonitorHead::max_attr_id_)
    {
        layout_->head_.MonitorHead::max_attr_id_=attr_id;
    }
//    update_index(attr);
    MinData & mindata=attr.data_[attr.index_];
    mindata.value_=(int64_t)val;
}


void Api::update_index(Attr & attr)
{
    int now_min=time(NULL)/60;
    int old_index=attr.index_;
    if(now_min > attr.data_[old_index].min_)
    {
        int new_index=(old_index+1)%RESERVE_MIN_CNT;
        attr.data_[new_index].min_=now_min;
        attr.data_[new_index].value_=0;
        __sync_bool_compare_and_swap(&(attr.index_), old_index, new_index);
    }

}

}//monitor
}//poseidon


