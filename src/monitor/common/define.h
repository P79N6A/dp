/**
 **/
namespace poseidon
{

namespace monitor
{

#define MONITOR_KEY 0x1234

#define MAX_ATTR_ID 10000000
#define RESERVE_MIN_CNT 3
#define USED_FLAG 255

#pragma pack(1)

struct MonitorHead
{
    int max_attr_id_;    //最大的属性ID
};

/*分钟数据*/
struct MinData
{
    int min_;               //分钟seq,(unix_time(sec)/60)
    int64_t value_;         //value
};

struct Attr
{
    int usedflag_;                  //是否正在使用, 如果在使用，为USED_FLAG, 否则没用到
    int update_time_;               //更新index时间
    int index_;
    MinData data_[RESERVE_MIN_CNT];
};

struct MonitorBody
{
    Attr attr_[MAX_ATTR_ID];
};

struct MonitorLayOut
{
    MonitorHead head_;
    MonitorBody body_;
};
#pragma pack()

}//monitor
}//poseidon
