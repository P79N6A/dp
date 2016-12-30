/**
 **/

#include <iostream>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <queue> 

struct Ad
{
    std::string name;
};

#define MAP std::map

typedef int TagType;

struct Node
{
    std::vector<TagType> vrkey;
    std::list<Ad> list_;
    std::set<Node*> children_;
};

class MyIndex
{
public:

    enum
    {
        MAX_TARGET_CNT=64,
    };

    int parse_from_file(const char * filename);

    int query(std::vector<TagType> req,  std::set<Ad> &res);

    int print_debug_str();

private:
    int parse_line(char * buf, int size, Ad & ad, std::vector<TagType> &vr);

    bool ischild(const std::vector<TagType> & vr1, const std::vector<TagType>& vr2);

    int split(std::string str, std::vector<TagType> & vr);

    int get_result(Node * pnode, std::set<Ad> &res, int rflag);
    

private:
    MAP<std::vector<TagType>, Node * > map_graph_;

    MAP<int , std::list<Node *> > map_size_list_;

    MAP<int , MAP< TagType, std::list<Node *> > > map_list_;

    /**************************************************
     * size:
     * 5
     * 4------->TagValA---->NodeList
     *      |-->TagValB---->NodeList
     *      |-->TagValC---->NodeList
     * 3
     * 2
     * 1
     * 0
     ************************************************/


    
};
