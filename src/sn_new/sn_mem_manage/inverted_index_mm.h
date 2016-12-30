/**
 **/


namespace poseidon
{
namespace sn
{

/**
 * @brief       倒排索引内存管理
 **/
class InvertedIndexMm
{
public:

    int init();
    int add_ad();
    int flush_to_shm();

    int query( Query )
    {
        //for()
    }

private:

    struct
    {
//        std::map<int, int64_t> map_tag;                         //size-> <size>_<tag>_xxx;
        std::map<std::string, std::list<Conj> > map_tag_conj;   //<size>_<tag>->conj_list
        std::map<Conj, Adlist>;                                 //
    };
};
}
}
