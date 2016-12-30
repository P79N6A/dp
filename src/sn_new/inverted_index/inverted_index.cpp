/**
 **/

#include "src/sn_new/inverted_index/inverted_index.h"

#include "util/util_str.h"

#include "util/func.h"

namespace poseidon {
namespace sn {

int InvertedIndex::Init(int dataid)
{
    int rt = 0;
    do {
        if (!kvapi_.init()) {
            rt = -1;
            break;
        }
        dataid_ = dataid;
    } while (0);
    return rt;
}

int InvertedIndex::GetSizeTagSet(int size, std::set<TagType> &tag_set)
{
    int rt = 0;
    do {
        inverted_index::SizeTagList size_tag_list;
        util::UtilStr ustr;
        std::string key = ustr.format("s_t_l_%d", size).str();
        std::string value;
        rt = kvapi_.get(dataid_, key, value);
        if (rt != 0) {
            break;
        }
        if (!size_tag_list.ParseFromString(value)) {
            rt = -1;
            break;
        }
        int tag_size = size_tag_list.tag_size();
        for (int i = 0; i < tag_size; i++) {
            tag_set.insert(size_tag_list.tag(i));
        }
    } while (0);
    return rt;
}

int InvertedIndex::QueryIndex(const Query &query,
                              std::list<common::Ad> &ad_list)
{
    int rt = 0;
    do {
        int nrt;
        int query_size = query.size();
        int max_size = query_size;

        std::string max_conf_size_key = "max_conj_size";
        std::string mvalue;
        nrt = kvapi_.get(dataid_, max_conf_size_key, mvalue);
        if (nrt == 0) {
            max_size = util::Func::to_int(mvalue);
        }

        for (int size = max_size; size > 0; size--) {
            std::map<std::string, int> map_conj_cnt;

            std::set<TagType> set_tag;
            nrt = GetSizeTagSet(size, set_tag);
            if (nrt != 0) {
                continue;
            }
            Query::const_iterator cit;
            for (cit = query.begin(); cit != query.end(); ++cit) {
                //没有的话, 调过
                if (set_tag.count(*cit) == 0) {
                    continue;
                }
                util::UtilStr ustr;
                std::string key = ustr.format("%d_%llu_s_t", size, *cit).str();
                std::string value;
                nrt = kvapi_.get(dataid_, key, value);
                if (nrt != 0) {
                    continue;
                }
                inverted_index::SizeTagValue stvalue;
                if (!stvalue.ParseFromString(value)) {
                    continue;
                }

                /*统计每个conj_key出现的次数*/
                int conj_key_size = stvalue.conj_key_size();
                for (int index = 0; index < conj_key_size; ++index) {
                    map_conj_cnt[stvalue.conj_key(index)]++;
                }
            }
            std::map<std::string, int>::iterator mit;
            for (mit = map_conj_cnt.begin(); mit != map_conj_cnt.end(); mit++) {
                /*检索到的Conj*/
                if (mit->second == size) {
                    std::string value;
                    if (kvapi_.get(dataid_, mit->first, value) != 0) {
                        continue;
                    }
                    inverted_index::ConjInfo conj_info;
                    if (!conj_info.ParseFromString(value)) {
                        continue;
                    }
                    int ad_size = conj_info.ad_size();
                    for (int ad_index = 0; ad_index < ad_size; ad_index++) {
                        ad_list.push_back(conj_info.ad(ad_index));
                    }
                }
            }
        }
        std::string value;
        std::string key = "conj_null";
        if (kvapi_.get(dataid_, key, value) == 0) {
            inverted_index::ConjInfo conj_info;
            if (!conj_info.ParseFromString(value)) {
                continue;
            }
            int ad_size = conj_info.ad_size();
            for (int ad_index = 0; ad_index < ad_size; ad_index++) {
                ad_list.push_back(conj_info.ad(ad_index));
            }
        }

    } while (0);
    return rt;
}
}
}
