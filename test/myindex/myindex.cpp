/**
 **/

#include "myindex.h"
#include "stdio.h"
#include "string"
#include "algorithm"
#include "string.h"
#include "stdlib.h"
#include "sstream"

#define LOG_DEBUG(fmt, a...) fprintf(stderr, fmt, ##a )

std::string vrtostr(std::vector<TagType> vr)
{

    std::vector<int>::iterator it;
    std::stringstream ss;
    ss<<"(";
    for(it=vr.begin(); it!=vr.end(); it++)
    {
        ss<<*it<<",";
    }
    ss<<")";
    return ss.str();

}

int MyIndex::split(std::string str, std::vector<TagType> & vr)
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
            vr.push_back(tagid);
        }
        oldpos=pos+1;
    }
    std::sort(vr.begin(), vr.end());
    return 0;
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

int MyIndex::parse_line(char * buf, int size, Ad & ad, std::vector<TagType> &vr)
{
    int rt=0;
    do{
        //buf format: name\ttagid1,tagid2,...,NULL
        char buf1[256];
        char buf2[512];
        sscanf(buf, "%s%s", buf1, buf2);
        ad.name=buf1;
        split(buf2, vr);
    }while(0);
    return rt;
}

int MyIndex::parse_from_file(const char * filename)
{
    int rt=0;
    FILE * fp=NULL;
    do{
        fprintf(stderr, "build from file[%s]...\n", filename);
        fp=fopen(filename, "r+");
        if(fp == NULL)
        {
            rt=-1;
            break;
        }
        char buf[1024];
        while(1)
        {
            memset(buf, 0x00, 1024);
            if(fgets(buf, 1024, fp)==NULL)
            {
                break;
            }
            /* step 1: parse line */
            Ad ad;
            std::vector<TagType> vr;
            rt=parse_line(buf, 1024, ad, vr);
            if(rt != 0)
            {
                break;
            }
            /*step 2:判断Node是否存在*/
            if(map_graph_.count(vr) > 0)
            {//已经存在，把Ad加到已有Node就Ok了
                map_graph_[vr]->list_.push_back(ad);
            }else
            {
                Node * pn=new Node();
                pn->vrkey=vr;
                pn->list_.push_back(ad);

                int vr_size=vr.size();

                /* pn加到所有父节点的child节点中*/
                for(int i=vr_size+1; i<MAX_TARGET_CNT; i++)
                {
                    if(map_size_list_.count(i) > 0)
                    {
                        std::list<Node *>::iterator it;
                        for(it=map_size_list_[i].begin();
                            it != map_size_list_[i].end();
                            it++)
                        {
                            if(ischild(vr, (*it)->vrkey))
                            {
                                (*it)->children_.insert(pn);
                            }

                        }
                    }
                }

                /* pn所有子节点加到pn的children中*/
                for(int j=0; j< vr_size; j++ )
                {
                    if(map_size_list_.count(j) > 0)
                    {
                        std::list<Node *>::iterator it;
                        for(it=map_size_list_[j].begin();
                            it != map_size_list_[j].end();
                            it++)
                        {
                            if(ischild((*it)->vrkey, vr))
                            {
                                (*it)->children_.insert(pn);
                                pn->children_.insert(*it);
                            }

                        }
                    }
                }

                map_graph_[vr]=pn;//create vr node
                map_size_list_[vr_size].push_back(pn);

                /**/
                for(int i=0; i< vr_size; i++)
                {
                    map_list_[vr_size][vr[i]].push_back(pn);
                }

            }


        }
        fprintf(stderr, "build from file[%s]done\n", filename);
    }while(0);
    if(fp!=NULL)
    {
        fclose(fp);
    }
    return rt;
}

int MyIndex::get_result(Node * pnode, std::set<Ad> &res, int rflag)
{
    int rt=0;
    do{
        std::list<Ad>::iterator it;
        for(it = pnode->list_.begin(); it != pnode->list_.end(); it++)
        {
            if(res.count(*it)==0)
            {
                res.insert(*it);
            }
        }
        if(rflag)
        {
            std::set<Node*>::iterator itn;
            for(itn=pnode->children_.begin();
                itn !=pnode->children_.end();
                itn++)
            {
                get_result(*itn, res, 0);
            }
        }
    }while(0);
    return rt;
}

#if 0
int fenlie(std::vector<int> base, std::set< std::vector<int> > & setvr )
{
    int size=base.size();
    for(int i=0; i<size; i++)
    {
        std::vector<int> vr;
        for(int j=i;j<size;j++)
        {
            vr.push_back(base[j]);
            setvr.insert(vr);
        }
    }
    return 0;
}
#endif

int MyIndex::query(std::vector<TagType> req,  std::set<Ad> &res)
{
    int rt=0;
    do{
        if(req.size() > MAX_TARGET_CNT)
        {
            rt=-1;
            break;
        }
        /*排序加快匹配速度*/
        std::sort(req.begin(), req.end());

        if(map_graph_.count(req)>0)
        {
            Node * pnode=map_graph_[req];
            get_result(pnode, res, 1);
        }else
        {
//            fenlie(req, setvr);
            int size=req.size();
            std::set<std::vector<int> > setvr;
            for(int idx=size-1; idx>=0; idx--)
            {
                if(map_size_list_.count(idx)==0)
                {
                    continue;
                }
                std::map<std::vector<TagType>, int> mapvrcnt;
                MAP<TagType, std::list<Node *> > & maplist=map_list_[idx];
                std::vector<TagType>::iterator it;
                for(it=req.begin(); it!=req.end(); it++)
                {//遍历每一个属性
                    if(maplist.count(*it) > 0)
                    {
                        std::list<Node *> & list=maplist[*it];
                        std::list<Node *>::const_iterator itlist;
                        for(itlist=list.begin(); itlist != list.end(); itlist++)
                        {
                            mapvrcnt[(*itlist)->vrkey]++;
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
                if(map_graph_.count(*itset) > 0)
                {
                    Node * pnode=map_graph_[*itset];
                    get_result(pnode, res, 0);
                }
            }
            std::vector<TagType> emptyvr;
            if(map_graph_.count(emptyvr)>0)
            {
                Node * pnode=map_graph_[emptyvr];
                get_result(pnode, res, 0);
            }


        }
        
    }while(0);
    return rt;
}
bool operator<(Ad a, Ad b)
{
    return a.name< b.name;
}

int MyIndex::print_debug_str()
{
    return 0;
#if 0
    std::map<std::vector<int>, Node * >::iterator it;
    for(it=map_graph_.begin(); it != map_graph_.end(); it++)
    {
        std::cout<<
    }
#endif
}

#ifdef _TEST_
int main(int argc, char * argv[])
{
    int rt=0;
    do{
        MyIndex mi;
        rt=mi.parse_from_file("./ad.data");
        if(rt != 0)
        {
            printf("parse_from_file error[%d]\n", rt);
            break;
        }
        mi.print_debug_str();

        std::vector<int> vr;
        int i=1;
        while(i<argc)
        {
            vr.push_back(atoi(argv[i]));
            i++;
        }
        std::set<Ad> res;
        rt=mi.query(vr, res);
        if(rt != 0)
        {
            printf("parse_from_file error[%d]\n", rt);
            break;
        }
        std::set<Ad>::iterator it;
        for(it =res.begin(); it != res.end(); it++)
        {
            std::cout<<it->name<<std::endl;
        }

    }while(0);
    return rt;
}
#endif







