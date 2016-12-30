/**
 **/

#ifndef  _MYINDEX_H_ 
#define  _MYINDEX_H_


#include "json/json.h"

#include <iostream>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <queue> 
#include <stdint.h>
#include <boost/serialization/singleton.hpp>

namespace poseidon
{
namespace sn
{


#define MAP std::map

typedef int64_t TagType;
typedef std::vector<TagType> Conj;
typedef std::vector<TagType> Query;


enum
{
    POST_TYPE_NULL=0,
    POST_TYPE_ALL=1,
    POST_TYPE_HOUR=2,
};

enum
{
    CITY_TARG_ID=1007,
    VIEW_TYPE_TARG_ID=19001,
    SOURCE_TARG_ID=19002,
    DEAL_ID_TARG_ID=19003,
};

enum
{
    INT_UNINIT=-999999,
};

struct IndexAd
{
    uint64_t adid;                  //广告ID

    uint32_t campaign_id;           //计划组id

    int64_t freq_impression;        //独立访客展现量控制

    int64_t campaign_daily_budget;  //每日预算, -1, 表示不限

    uint32_t send_speed;            //投放策略

    std::string billing_type;       //竞价类型： T:CPT, M:CPM  C:CPC  
    int ad_time_type;               //投放时间类型：1-全时间段，2-特定时间段

    uint64_t post_hours;            //计划投递时段, counter拿到这个字段要做预算的计算和平滑,每一个位表示一个时段，,每个时段半小时, 共48个时段

    uint32_t advertiser_id;         //广告主id

    uint32_t advertiser_budget;     //广告主预算

    uint32_t adgroup_id;            //推广单元id

    uint64_t creative_id;           //创意id, 经过了templateid, category, level, format等过滤之后的

    uint32_t org_price;             //广告主原始报价

    uint32_t algo_price;            //算法报价

    uint32_t score;                 //索引离线海选分数

    uint32_t creative_format;       //创意类型 1. 图片 2 文字 3 flash 4 video等 

    uint32_t base_price;            //广告主想要拿下流量的基础出价，算法报价在 org_price(最高报价)之下，在base_price左右浮动

    uint32_t bid_type;              //报价方式 1: 智能出价; 2: 固定报价;
    uint32_t product;               //付费方式

    std::string ch;                 //渠道
    
    uint32_t inner_advertiser_id;   //内部广告主ID

    uint32_t gid;                   //gid


    std::set<int> view_types;       //广告投放平台

    int source;                     //渠道

    int32_t premium_rate;           //溢价率，百分数，example:20表示溢价20% 

    int32_t advertiser_balance;     //广告主余额

    std::string advertiser_balance_day; //广告主余额的天,格式"YYYYMMDD", 如“20161109”


//pdb 特有 begin 
    std::string deal_id;            //订单ID
    int settle_price;               //媒体结算价格
    int64_t total_exp;              //订单总曝光量
    int64_t day_exp;                //订单当天曝光数
    int campaign_quota;             //推广计划份额
    int fill_rate;                  //填充率
    int campaign_type;              //推广计划类型
//pdb 特有 end

    std::vector<uint32_t> city_id;//城市定向,可能有多个ID

    Conj conj;                      //定向数据

    IndexAd()
    {
        adid=0;
        campaign_id=0;
        freq_impression=0;
        campaign_daily_budget=0;
        send_speed=0;
        ad_time_type=0;
        post_hours=0;
        advertiser_id=0;
        advertiser_budget=0;
        adgroup_id=0;
        creative_id=0;
        org_price=0;
        algo_price=0;
        score=0;
        creative_format=0;
        base_price=0;
        bid_type=0;
        product=0;
        inner_advertiser_id=0;
        gid=0;
        source=0;
        settle_price=0;
        total_exp=0;
        day_exp=0;
        campaign_quota=0;
        fill_rate=0;
        campaign_type=0;
        premium_rate=INT_UNINIT;
        advertiser_balance=INT_UNINIT;
    }
};

struct Node
{
    Conj key;
    std::list<IndexAd> list_;
    std::set<Node*> children_;
};


/*表示一份倒排索引*/
struct LayOut
{
    //Conj-->Node
    MAP<std::vector<TagType>, Node * > map_graph_;

    //size-->NodeList
    //  |
    // 4|->Node->Node->Node...
    // 3|->Node->Node->Node...
    // 2|->Node->Node->Node...
    // 1|->Node->Node->Node...
    // 0|->Node->Node->Node...
    MAP<int , std::list<Node *> > map_size_list_;   //size-->NodeList

    //size->MAP(tag->NodeList)
    //  |
    // 4|---|
    //  |   tag1->Node->Node->Node...
    //  |   tag2->Node->Node->Node...
    //  |   tag3->Node->Node->Node...
    // 3|---|
    //  |   tag1->Node->Node->Node
    //  |   tag2->Node....
    //  |   ...
    // 2|   ...
    //  |
    // 1|   ...
    //  |
    // 0|   ...
    //
    MAP<int , MAP< TagType, std::list<Node *> > > map_list_;
    void reset(){
        MAP<std::vector<TagType>, Node * >::iterator it;
        for(it=map_graph_.begin(); it!=map_graph_.end(); it++)
        {
            if(it->second != NULL)
            {
                delete it->second;
                it->second=NULL;
            }
        }
        map_graph_.clear();
        map_size_list_.clear();
        map_list_.clear();
    }

};

class TargetInput
{
public:
    virtual int init()=0;
    virtual bool get_one_ent(IndexAd & ad, std::vector<Conj> & vrconj)=0; 
    virtual void close()=0;    
};

class FileInput:public TargetInput
{

public:
    virtual int init();
    virtual bool get_one_ent(IndexAd & ad, std::vector<Conj> & vrconj); 
    virtual void close();
private:
    int parse_line(char * buf, int size, IndexAd & ad, std::vector<Conj> &vrconj);
    int split(std::string str, Conj & conj);
    int get_index_data_file(std::string & index_file);

    Json::Reader reader_;
    FILE * fp_;
};


class MyIndex:public boost::serialization::singleton<MyIndex>
{
public:


    enum
    {
        MAX_TARGET_CNT=32,
        MAX_QUERY_SIZE=512,
    };


    MyIndex():layout_idx_(0), input_(NULL),allow_switch_(1){}


    /**
     * @brief   启动更新线程
     **/
    int start_update_thread();

    typedef void ThreadFn(MyIndex *);



    /**
     * @brief               更新线程执行的线程函数
     **/
    static void update_thread_fn(MyIndex * ins );

    int update_index();

    /**
     * @brief               增加一个广告位
     * @param ad            [IN], 输入广告
     * @param conj          [IN], 广告的定向
     * @return              success return 0, or return other code
     **/
    int add_ad(const IndexAd & ad, Conj conj);

    /**
     * @brief               查询，输入一组用户定向，返回广告列表
     * @param req           [IN], 查询的请求条件
     * @param res           [IN], 返回广告的集合
     * @return              success return 0, or return other code
     * */
    int query(Query req,  std::set<IndexAd> &res);


    void set_input(TargetInput * input)
    {
        input_=input;
    }
    
    /**
     * @brief               更新layout内容
     **/
    int update_layout();

private:

    bool ischild(const std::vector<TagType> & vr1, const std::vector<TagType>& vr2);


    /**
     * @brief               从一个Node获取广告的结果数据
     * @param pnode         [IN], 输入的Node节点
     * @param res           [OUT], 输出的结果追加到res中
     * @param rflag         [IN], 是否递归到子节点
     **/
    int get_result(Node * pnode, std::set<IndexAd> &res, int rflag);


    /**
     * @brief               转换layout
     **/
    void switch_layout()
    {
        while(1)
        {
            if(allow_switch_)
            {
                layout_idx_=(layout_idx_+1)%2;
                break;
            }
        }
    }


    /**
     * @brief           return read layout
     **/
    LayOut & RL()
    {
        return layout_[layout_idx_];
    }

    /**
     * @brief           return write layout
     **/
    LayOut & WL()
    {
        return layout_[(layout_idx_+1)%2];
    }

private:
    LayOut layout_[2];
    int layout_idx_; 
    TargetInput * input_;
    volatile int allow_switch_;

#if 0
    MAP<std::vector<TagType>, Node * > map_graph_;

    MAP<int , std::list<Node *> > map_size_list_;

    MAP<int , MAP< TagType, std::list<Node *> > > map_list_;

    /**************************************************
     * size:
     * 5
     * 4----|-->TagValA---->NodeList
     *      |-->TagValB---->NodeList
     *      |-->TagValC---->NodeList
     * 3
     * 2
     * 1
     * 0
     ************************************************/
#endif
};


}//sn
}//poseidon

#endif   // ----- #ifndef _MYINDEX_H_  ----- 
