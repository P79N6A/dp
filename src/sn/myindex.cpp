/**
 **/


#include "myindex.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <vector>
#include "json/json.h"
#include "config.h"
#include "util/log.h"
#include "util/func.h"
#include "util/thread_adapter.h"
#include "util/monitor.h"
#include "boost/thread.hpp"
#include "monitor_api.h"
#include "sn_attr.h"

namespace poseidon{
namespace sn{

std::string vrtostr(std::vector<TagType> vr)
{

    std::vector<TagType>::iterator it;
    std::stringstream ss;
    ss<<"(";
    for(size_t i=0; i<vr.size(); i++)
    {
        ss<<"{"<<(vr[i]>>32)<<":"<<(vr[i]&0xffff)<<"}"<<",";
    }
    ss<<")";
    return ss.str();

}


bool MyIndex::ischild(const std::vector<TagType> & vr1, const std::vector<TagType> &vr2)
{
    bool rt=true;
    do{
        //
        if(vr2.size()<= vr1.size())
        {
            rt=false;
            break;            
        }
        std::vector<TagType>::const_iterator it;
        //注意：vr里面的元素是排好序的
        for(it=vr1.begin(); it!=vr1.end(); it++)
        {
            rt=false;
            std::vector<TagType>::const_iterator it2;
            for(it2=vr2.begin(); it2!=vr2.end(); it2++)
            {
                if(*it==*it2)
                {
                    rt=true;
                    break;
                }
                if(*it < *it2)
                {
                    rt=false;
                    break;
                }
            }
            if(!rt)
            {
                break;
            }
        }

    }while(0);
    return rt;
}

/**
 * @brief               增加一个广告位
 * @param ad            [IN], 输入广告
 * @param conj          [IN], 广告的定向
 * @return              success return 0, or return other code
 **/
int MyIndex::add_ad(const IndexAd & ad, Conj conj)
{
    int rt=0;
    do{
        std::sort(conj.begin(), conj.end());

        /*step 2:判断Node是否存在*/
        if(WL().map_graph_.count(conj) > 0)
        {//已经存在，把Ad加到已有Node就Ok了
            WL().map_graph_[conj]->list_.push_back(ad);
        }else
        {
            Node * pn=new Node();
            pn->key=conj;
            pn->list_.push_back(ad);
            int conj_size=conj.size();

            /* pn加到所有父节点的child节点中*/
            /* 只遍历conj的大小大于当前conj的大小的*/
            for(int i=conj_size+1; i<MAX_TARGET_CNT; i++)
            {
               if(WL().map_size_list_.count(i) > 0)
               {
                  std::list<Node *>::iterator it;
                  for(it=WL().map_size_list_[i].begin();
                      it != WL().map_size_list_[i].end();
                      it++)
                  {
                      if(ischild(conj, (*it)->key))
                      {
                          (*it)->children_.insert(pn);
                      }

                  }
               }
            }

            /* pn所有子节点加到pn的children中*/
            /* 只遍历conj的大小小于当前conj的大小的*/
            for(int j=0; j< conj_size; j++ )
            {
                if(WL().map_size_list_.count(j) > 0)
                {
                    std::list<Node *>::iterator it;
                    for(it=WL().map_size_list_[j].begin();
                        it != WL().map_size_list_[j].end();
                        it++)
                    {
                        if(ischild((*it)->key, conj))
                        {
                            (*it)->children_.insert(pn);
                            pn->children_.insert(*it);
                        }

                    }
                }
            }

            WL().map_graph_[conj]=pn;//create conj node
            WL().map_size_list_[conj_size].push_back(pn);

            /*TODO:加注释*/
            for(int i=0; i< conj_size; i++)
            {
                WL().map_list_[conj_size][conj[i]].push_back(pn);
            }

        }
    }while(0);
    return rt;
}


int MyIndex::get_result(Node * pnode, std::set<IndexAd> &res, int rflag)
{
    int rt=0;
    do{
        if(res.size() >= MAX_QUERY_SIZE)
        {
            break;
        }
        std::list<IndexAd>::iterator it;
        for(it = pnode->list_.begin(); it != pnode->list_.end(); it++)
        {
            if(res.count(*it)==0)
            {
                res.insert(*it);
                if(res.size() >= MAX_QUERY_SIZE)
                {
                    break;
                }
            }
        }
        if(res.size() >= MAX_QUERY_SIZE)
        {
            break;
        }
        if(rflag)
        {
            std::set<Node*>::iterator itn;
            for(itn=pnode->children_.begin();
                itn !=pnode->children_.end();
                itn++)
            {
                if(res.size() >= MAX_QUERY_SIZE)
                {
                    break;
                }
                get_result(*itn, res, 0);
            }
        }
    }while(0);
    return rt;
}

int MyIndex::query(std::vector<TagType> req,  std::set<IndexAd> &res)
{
    MON("MYINDEX_QUERY", 1);
    int rt=0;
    do{
        if(req.size() > MAX_TARGET_CNT)
        {
            rt=-1;
            break;
        }
        /*排序加快匹配速度*/
        std::sort(req.begin(), req.end());

        allow_switch_=0;//不允许switch

        //查询的时候，禁止switch layout, 防止前后的RL不一致
        if(RL().map_graph_.count(req)>0)
        {
            Node * pnode=RL().map_graph_[req];
            get_result(pnode, res, 1);
        }else
        {
            int size=req.size();
            std::set<std::vector<TagType> > setvr;
            for(int idx=size-1; idx>=0; idx--)
            {
                if(RL().map_size_list_.count(idx)==0)
                {
                    continue;
                }
                std::map<std::vector<TagType>, int> mapvrcnt;
                MAP<TagType, std::list<Node *> > & maplist=RL().map_list_[idx];
                std::vector<TagType>::iterator it;
                for(it=req.begin(); it!=req.end(); it++)
                {//遍历每一个属性
                    if(maplist.count(*it) > 0)
                    {
                        std::list<Node *> & list=maplist[*it];
                        std::list<Node *>::const_iterator itlist;
                        for(itlist=list.begin(); itlist != list.end(); itlist++)
                        {
                            mapvrcnt[(*itlist)->key]++;
                        }
                    } 
                }

                std::map<std::vector<TagType>, int>::iterator itmap;
                for(itmap=mapvrcnt.begin(); itmap!=mapvrcnt.end(); itmap++)
                {
                    if(itmap->second == idx)
                    {
                        setvr.insert(itmap->first);
                    }
                }

            }
            std::set<std::vector<TagType> >::iterator itset;
            for(itset=setvr.begin(); itset != setvr.end(); itset++)
            {
                if(RL().map_graph_.count(*itset) > 0)
                {
                    Node * pnode=RL().map_graph_[*itset];
                    get_result(pnode, res, 0);
                    if(res.size() >= MAX_QUERY_SIZE)
                    {
                        break;
                    }
                }
            }
            if(res.size() >= MAX_QUERY_SIZE)
            {
                break;
            }
            std::vector<TagType> emptyvr;
            if(RL().map_graph_.count(emptyvr)>0)
            {
                Node * pnode=RL().map_graph_[emptyvr];
                get_result(pnode, res, 0);
            }
        }
    }while(0);
    allow_switch_=1;
    return rt;
}
int MyIndex::update_index()
{
    int rt=0;
    MON("UPDATE_INDEX_CNT", 1);
    FILE * fp=NULL;
    do{
        std::string index_done_file=Config::get_mutable_instance().index_data_done_file();
        std::string index_data_path=Config::get_mutable_instance().index_data_path();
        fp=fopen(index_done_file.c_str(), "r+");
        if(fp == NULL)
        {
//            LOG_ERROR("open index_done_file error[%s]", index_done_file.c_str() );
            break;
        }
        char buf[256];
        memset(buf, 0x00, 256);
        if(fgets(buf, 256, fp)==NULL)
        {
            break;
        }
        char md5sum[128];
        char index_file[128];
        sscanf(buf, "%s%s", md5sum, index_file );
//        LOG_DEBUG("md5sum[%s]index_file[%s]", md5sum, index_file);
        std::string index_data_file=index_data_path+"/"+index_file;
        //check data file
        std::string md5str; 
        rt=util::Func::md5sum(index_data_file, md5str);
        if(rt != 0 )
        {
            LOG_ERROR("cal md5sum error filename[%s]", index_data_file.c_str());
            rt=-1;
            break;
        }
        if(md5str != md5sum )
        {
            LOG_ERROR("md5sum check error,file[%s]md5sum[%s]check_md5sum[%s]",
                    index_data_file.c_str(),
                    md5sum,
                    md5str.c_str());
            MON_ADD(ATTR_MD5_ERROR, 1);
            rt=-2;
            break;
        }
        
        Config::get_mutable_instance().set_index_data_file(index_data_file.c_str());
        update_layout();

    }while(0);
    if(fp != NULL)
    {
        fclose(fp);
    }
    return rt;
}

/**
 * @brief               更新线程执行的线程函数
 **/
void MyIndex::update_thread_fn(MyIndex * ins)
{
    std::string index_done_file=Config::get_mutable_instance().index_data_done_file();

    int last_modify_time=time(NULL);//初始化文件修改时间为当前时间
    int nrt=0;
    while(1)
    {
        sleep(10);//1秒钟检查一次
        //检查文件时间
        struct stat st;
        nrt=stat(index_done_file.c_str(), &st);
        if(nrt < 0)
        {
//            LOG_ERROR("stat error[%s]", strerror(errno));
            continue;
        }
        if(st.st_mtime <= last_modify_time)
        {
            continue;
        }
        last_modify_time=st.st_mtime;
        if(ins->update_index()!=0)
        {
            MON_ADD(ATTR_UPDATE_ERROR, 1);
        }
    }
}

/**
 * @brief   启动更新线程
 **/
int MyIndex::start_update_thread()
{
    int rt=0;
    do{
        boost::thread * pth =new boost::thread(util::Adapter<ThreadFn, MyIndex *>(update_thread_fn, this));
        pth->detach();
    }while(0);
    return rt;
}


bool operator<(IndexAd a, IndexAd b)
{
    return a.adid< b.adid;
}

int MyIndex::update_layout()
{
    int rt=0;
    do{
        MON_ADD(ATTR_SN_UPDATE_INDEX, 1);
        //TODO:start a thread to do this
//        LOG_DEBUG("start update_layout!");
        if(input_ == NULL)
        {
            LOG_ERROR("input is null");
            rt=-1;
            break;
        }

        WL().reset();
        rt=input_->init();
        if(rt != 0)
        {
            LOG_ERROR("input init error,rt[%d]", rt);
            break;
        }
        int record_cnt=0;
        int conj_cnt=0;
        while(1)
        {
            std::vector<Conj> vrconj;
            IndexAd ad;

            /*一个广告可能有多个定向*/
            if(input_->get_one_ent(ad, vrconj))
            {
                record_cnt++;
                std::vector<Conj>::iterator it;
                for(it=vrconj.begin(); it != vrconj.end(); it++)
                {
                    conj_cnt++;
//                    LOG_DEBUG("insert conj[%s]\n", vrtostr(*it).c_str());
                    add_ad(ad, *it);
                }
            }else
            {
                break;
            }
        }
        input_->close();

//        LOG_DEBUG("update_layout finish, record cnt[%d]conj_cnt[%d]", record_cnt, conj_cnt );
        switch_layout();
    }while(0);
    return rt;
}

int FileInput::get_index_data_file(std::string & index_file)
{
    int rt=0;
    do{
        index_file=Config::get_mutable_instance().index_data_file();
    }while(0);
    return rt;
}

int FileInput::init()
{
    int rt=0;
    do{
        std::string indexdatafile;
        rt=get_index_data_file(indexdatafile);
        if(rt != 0)
        {
            rt=-1;
            break;
        }
        fp_=fopen(indexdatafile.c_str(), "r");
        if(fp_ == NULL)
        {
            rt=-1;
            break;
        }
    }while(0);
    return rt;
}

int FileInput::split(std::string str, Conj & conj)
{
    size_t pos=0;
    size_t oldpos=0;
    while(1)
    {
        pos=str.find(",", oldpos);
        if(pos == std::string::npos)
        {
            break;
        }
        std::string substr=str.substr(oldpos, pos-oldpos);
        int tagid=atoi(substr.c_str());
        if(tagid > 0)
        {
            conj.push_back(tagid);
        }
        oldpos=pos+1;
    }
    std::sort(conj.begin(), conj.end());
    return 0;
}

int FileInput::parse_line(char * buf, int size, IndexAd & ad, std::vector<Conj> &vrconj)
{
    int rt=0;
    do{
        try
        {
            Json::Value root;
            if(!reader_.parse(buf, root, false))
            {
                LOG_ERROR("json parse error");
                rt=-1;
                break;
            }

            if(!root["uuid"].isNull())
            {
                ad.adid=root["uuid"].asInt();
            }
            if(!root["campaign_id"].isNull())
            {
                ad.campaign_id=root["campaign_id"].asInt();
            }
            if(!root["freq_impression"].isNull())
            {
                ad.freq_impression=root["freq_impression"].asInt();
            }
            if(!root["campaign_daily_budget"].isNull())
            {
                ad.campaign_daily_budget=root["campaign_daily_budget"].asInt();
            }
            if(!root["send_speed"].isNull())
            {
                ad.send_speed=root["send_speed"].asInt();
            }
            if(!root["advertiser_id"].isNull())
            {
                ad.advertiser_id=root["advertiser_id"].asInt();
            }
            if(!root["advertiser_budget"].isNull())
            {
                ad.advertiser_budget=root["advertiser_budget"].asInt();
            }
            if(root["creative_id"].isArray())
            {
                ad.creative_id=root["creative_id"][0].asInt();
            }
            if(!root["org_price"].isNull())
            {
                ad.org_price=root["org_price"].asInt();
            }
            if(!root["creative_format"].isNull())
            {
                ad.creative_format=root["creative_format"].asInt();
            }
            if(!root["bid_type"].isNull())
            {
                ad.bid_type=root["bid_type"].asInt();
            }

            if(!root["adgroup_id"].isNull() )
            {
                ad.adgroup_id=root["adgroup_id"].asInt();
            }

            if(!root["billing_type"].isNull() )
            {
                ad.billing_type=root["billing_type"].asString();
            }

            if(!root["product"].isNull() )
            {
                ad.product=root["product"].asInt();
            }
            if(root["ad_platform"].isArray() )
            {
                int view_type_size=root["ad_platform"].size();
                for(int i=0; i < view_type_size; i++ )
                {
                    ad.view_types.insert(root["ad_platform"][i].asInt());
                }
            }
            if(!root["source"].isNull())
            {
                ad.source=root["source"].asInt();
            }

            if(root["premium_rate"].isInt())
            {
                ad.premium_rate=root["premium_rate"].asInt();
            }
            if(root["advertiser_balance"].isInt())
            {
                ad.advertiser_balance=root["advertiser_balance"].asInt();
            }
            if(root["advertiser_balance_day"].isString())
            {
                ad.advertiser_balance_day=root["advertiser_balance_day"].asString();
            }

            //pdb相关
            if(!root["pdb_data"].isNull())
            {

                if(!root["pdb_data"]["deal_id"].isNull())
                {
                    ad.deal_id=root["pdb_data"]["deal_id"].asString();
                }

                if(!root["pdb_data"]["settle_price"].isNull())
                {
                    ad.settle_price=root["pdb_data"]["settle_price"].asInt();
                }
                if(!root["pdb_data"]["total_exp"].isNull())
                {
                    ad.total_exp=root["pdb_data"]["total_exp"].asInt64();
                }
                if(!root["pdb_data"]["day_exp"].isNull())
                {
                    ad.day_exp=root["pdb_data"]["day_exp"].asInt64();
                }
                if(!root["pdb_data"]["campaign_quota"].isNull())
                {
                    ad.campaign_quota=root["pdb_data"]["campaign_quota"].asInt();
                }
                if(!root["pdb_data"]["fill_rate"].isNull())
                {
                    ad.fill_rate=root["pdb_data"]["fill_rate"].asInt();
                }
            }
            if(!root["campaign_type"].isNull())
            {
                ad.campaign_type=root["campaign_type"].asInt();
            }

            if(!root["ch"].isNull())
            {
                ad.ch=root["ch"].asString();
            }

            if(!root["inner_advertiser_id"].isNull())
            {
                ad.inner_advertiser_id=root["inner_advertiser_id"].asInt();
            }

            if(!root["gid"].isNull() )
            {
                ad.gid=root["gid"].asInt();
            }

            if(!root["ad_time_type"].isNull())
            {
                ad.ad_time_type=root["ad_time_type"].asInt();
                if(ad.ad_time_type == POST_TYPE_ALL)
                {//全时段投放
                    ad.post_hours=0xffffffffffff;//48个1
                }else
                {
                    if(!root["post_hours"].isNull())
                    {
                        std::string post_hours;
                        post_hours=root["post_hours"].asString();
                        /*48*7=336, 一周7天，半个小时一个时段*/
                        if(post_hours.length() != 336)
                        {
                            LOG_ERROR("post_hours error[%s]", post_hours.c_str() );
                            rt=-1;
                            break;
                        }
                        struct tm tminfo;
                        util::Func::get_time_info(tminfo);
                        int weekday=tminfo.tm_wday;
                        int start_index=weekday*48;
                        uint64_t hours=0;
                        for(int n=0; n<48; n++)
                        {
                            if(post_hours[start_index+n] == '1')
                            {
                                hours|=(((uint64_t)1)<<n);
                            }
                        }
                        ad.post_hours=hours;
                    }else
                    {
                        ad.post_hours=0;
                    }
                }
            }


            Conj conj;//push一个空的进去
            vrconj.push_back(conj);
            int idx=0;
            int conj_size=0;
            std::vector<std::vector<TagType> > vrtagtype;
            if(!root["targeted_package"].isNull())
            {
                std::string strtargeted=root["targeted_package"].asString();
                Json::Value tarroot;
                if(!reader_.parse(strtargeted, tarroot, false))
                {
                    LOG_ERROR("json parse error");
                    rt=-1;
                    break;
                }
                Json::Value::Members mem=tarroot.getMemberNames();
         
                Json::Value::Members::iterator it;
                for(it=mem.begin(); it!=mem.end(); it++)
                {

                    int type=atoi(it->c_str());
                    //城市ID不做为定向条件，因为分裂太厉害了
                    if(type==CITY_TARG_ID)
                    {
                        if(tarroot[*it].isArray() && tarroot[*it].size() > 0)
                        {
                            int size=tarroot[*it].size();
                            for(int j=0; j< size; j++)
                            {
//                                maptar[type].push_back(tarroot[*it][j].asInt());
                                int value=tarroot[*it][j].asInt();
                                ad.city_id.push_back(value);
                            }
                        }
                        continue;
                    }

                    if(tarroot[*it].isArray() && tarroot[*it].size() > 0)
                    {
                        std::vector<TagType> vrtag;
                        int size=tarroot[*it].size();
                        for(int j=0; j< size; j++)
                        {
//                            maptar[type].push_back(tarroot[*it][j].asInt());
                            int value=tarroot[*it][j].asInt();
                            TagType tag=((int64_t)type << 32)|(int64_t)value;
                            vrtag.push_back(tag);
                        }
                        vrtagtype.push_back(vrtag);
                        idx++;
                    }
                }
                conj_size=idx;
            }


            //把view_type 和source作为定向条件的一部分
            if(ad.view_types.size() > 0)
            {
                std::vector<TagType> vrtag;
                std::set<int>::iterator it;
                for(it=ad.view_types.begin(); it != ad.view_types.end(); it++)
                {
                    int value=*it;
                    TagType tag=((int64_t)VIEW_TYPE_TARG_ID<< 32)|(int64_t)value;
                    vrtag.push_back(tag);
                }
                vrtagtype.push_back(vrtag);
                conj_size++;
            }
            if(ad.source != 0)
            {
                std::vector<TagType> vrtag;
                TagType tag=((int64_t)SOURCE_TARG_ID<< 32)|(int64_t)ad.source;
                vrtag.push_back(tag);
                vrtagtype.push_back(vrtag);
                conj_size++;
            }
            if(!ad.deal_id.empty())
            {
                std::vector<TagType> vrtag;
                TagType tag=((int64_t)DEAL_ID_TARG_ID<< 32)|util::Func::to_int(ad.deal_id);
                vrtag.push_back(tag);
                vrtagtype.push_back(vrtag);
                conj_size++;
            }else
            {
                //非PDB，加deal_id为0的定向
                std::vector<TagType> vrtag;
                TagType tag=((int64_t)DEAL_ID_TARG_ID<< 32)|0;
                vrtag.push_back(tag);
                vrtagtype.push_back(vrtag);
                conj_size++;
            }

         
            for(idx=0; idx < conj_size; idx++)
            {
                int tagsize=vrtagtype[idx].size();
                size_t conjsize=vrconj.size();
                for(int j=0; j<tagsize; j++)
                {
                    if(j==tagsize-1)
                    {//最后一个，在已有的基础上增加标签
                        for(size_t k=0; k< conjsize; k++)
                        {
                            vrconj[k].push_back(vrtagtype[idx][j]);
                        }
                    }else
                    {//同一个type有多个标签值，分裂出新的Conj出来
                        for(size_t k=0; k< conjsize; k++)
                        {
                            Conj newconj=vrconj[k];
                            newconj.push_back(vrtagtype[idx][j]);
                            vrconj.push_back(newconj);
                        }
                    }
                }
            }
        

        }catch(const std::exception & e)
        {
            LOG_ERROR("get exception[%s]buf[%s]\n", e.what(), buf);
        }catch(...)
        {
            LOG_ERROR("get unknowned exception");
        }


    }while(0);
    return rt;
}
bool FileInput::get_one_ent(IndexAd & ad, std::vector<Conj> & vrconj)
{
    bool rt=true;
    do{
        char buf[8192];
        memset(buf, 0x00, 8192);
        if(fgets(buf, 8192, fp_)==NULL)
        {
            rt=false;
            break;
        }

        int nrt=parse_line(buf, 8192, ad, vrconj);
        if(nrt != 0)
        {
            LOG_ERROR("parse_line error[%s]", buf);
            rt=true;
            break;
        }

    }while(0);
    return rt;
}

void FileInput::close()
{
    fclose(fp_);
    fp_=NULL;
}

}//sn
}//poseidon

