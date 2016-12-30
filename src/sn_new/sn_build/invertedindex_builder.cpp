/**
 **/

#include "invertedindex_builder.h"

#include <stdio.h>

#include <iostream>
#include <string>
#include <sstream>

#include "json/json.h"

#include "util/util_str.h"
#include "util/func.h"
#include "src/sn_new/common/comm_macro.h"
#include "monitor_api.h"

#define LOG_ERROR(fmt, a...) fprintf(stderr, "[%d in %s]" fmt, __LINE__, __FILE__, ##a)

namespace poseidon {
namespace sn {

std::string InvertedIndexBuilder::Conj2Str(const Conj &conj)
{
    std::stringstream ss;
    Conj::const_iterator cit;
    if (conj.size() == 0) {
        ss << "conj_null";
    } else {
        ss << "conj_";
        for (cit = conj.begin(); cit != conj.end(); ++cit) {
            ss << *cit << "_";
        }
    }
    return ss.str();
}

int InvertedIndexBuilder::Init(const std::string &zk_host_list,
                               int data_servr_port)
{
    int rt = 0;
    do {
        rt = kvregapi_.init(zk_host_list, data_servr_port);
        if (rt != 0) {
            LOG_ERROR("kvregapi_.init return error");
            break;
        }
    } while (0);
    return rt;
}

int InvertedIndexBuilder::ParseLine(const char *buf, int size, common::Ad &ad,
                                    std::vector<Conj> &vrconj)
{
    int rt = 0;
    do {
        try
        {
            Json::Value root;
            if (!reader_.parse(buf, root, false)) {
                LOG_ERROR("json parse error");
                rt = -1;
                break;
            }

            std::set<int> view_types;
            int source = 0;
            std::string deal_id;
#if 0
            if(root["uuid"].isInt()) {
                ad.set_adgroup_id(root["uuid"].asInt());
            }
#endif

            if (root["campaign_id"].isInt()) {
                ad.set_campaign_id(root["campaign_id"].asInt());
            }
            if (root["freq_impression"].isInt64()) {
                ad.set_freq_impression(root["freq_impression"].asInt());
            }
            if (!root["campaign_daily_budget"].isNull()) {
                ad.set_campaign_daily_budget(
                    root["campaign_daily_budget"].asInt64());
            }
            if (!root["send_speed"].isNull()) {
                if (root["send_speed"].asInt() == rtb::SST_FAST) {
                    ad.set_send_speed(rtb::SST_FAST);
                } else {
                    ad.set_send_speed(rtb::SST_SMOOTH);
                }
            }
            if (!root["advertiser_id"].isNull()) {
                ad.set_advertiser_id(root["advertiser_id"].asInt());
            }
            if (!root["advertiser_budget"].isNull()) {
                ad.set_advertiser_budget(root["advertiser_budget"].asInt());
            }
            if (root["creative_id"].isArray()) {
                ad.set_creative_id(root["creative_id"][0].asInt());
            }
            if (!root["org_price"].isNull()) {
                ad.set_org_price(root["org_price"].asInt());
            }
            if (!root["creative_format"].isNull()) {
                ad.set_creative_format(root["creative_format"].asInt());
            }
            if (!root["bid_type"].isNull()) {
                ad.set_bid_type(root["bid_type"].asInt());
            }

            if (!root["adgroup_id"].isNull()) {
                ad.set_adgroup_id(root["adgroup_id"].asInt());
            }

            if (!root["billing_type"].isNull()) {
                ad.set_billing_type(root["billing_type"].asString());
            }

            if (!root["product"].isNull()) {
                ad.set_product(root["product"].asInt());
            }
            if (root["ad_platform"].isArray()) {
                int view_type_size = root["ad_platform"].size();
                for (int i = 0; i < view_type_size; i++) {
                    view_types.insert(root["ad_platform"][i].asInt());
                    ad.set_view_type(root["ad_platform"][i].asInt());
                }
            }
            if (!root["source"].isNull()) {
                source = root["source"].asInt();
            }

            if (root["premium_rate"].isInt()) {
                ad.set_premium_rate(root["premium_rate"].asInt());
            }
            if (root["advertiser_balance"].isInt()) {
                ad.set_advertiser_balance(root["advertiser_balance"].asInt());
            }
            if (root["advertiser_balance_day"].isString()) {
                ad.set_advertiser_balance_day(
                    root["advertiser_balance_day"].asString());
            }

            common::Ad_PdbData *pdb_data = ad.mutable_pdb_data();

            // pdb相关
            if (!root["pdb_data"].isNull()) {

                if (!root["pdb_data"]["deal_id"].isNull()) {
                    pdb_data->set_deal_id(
                        root["pdb_data"]["deal_id"].asString());
                    deal_id = root["pdb_data"]["deal_id"].asString();
                }

                if (!root["pdb_data"]["settle_price"].isNull()) {
                    pdb_data->set_settle_price(
                        root["pdb_data"]["settle_price"].asInt());
                }
                if (!root["pdb_data"]["total_exp"].isNull()) {
                    pdb_data->set_total_exp(
                        root["pdb_data"]["total_exp"].asInt64());
                }
                if (!root["pdb_data"]["day_exp"].isNull()) {
                    pdb_data->set_day_exp(
                        root["pdb_data"]["day_exp"].asInt64());
                }
                if (!root["pdb_data"]["campaign_quota"].isNull()) {
                    pdb_data->set_campaign_quota(
                        root["pdb_data"]["campaign_quota"].asInt());
                }
                if (!root["pdb_data"]["fill_rate"].isNull()) {
                    pdb_data->set_fill_rate(
                        root["pdb_data"]["fill_rate"].asInt());
                }
            }
            if (!root["campaign_type"].isNull()) {
                ad.set_campaign_type(root["campaign_type"].asInt());
            }

            if (!root["ch"].isNull()) {
                ad.set_ch(root["ch"].asString());
            }

            if (!root["inner_advertiser_id"].isNull()) {
                ad.set_inner_advertiser_id(root["inner_advertiser_id"].asInt());
            }

            if (!root["gid"].isNull()) {
                ad.set_gid(root["gid"].asInt());
            }

            if (!root["ad_time_type"].isNull()) {
                int ad_time_type = root["ad_time_type"].asInt();
                if (ad_time_type == POST_TYPE_ALL) {
                    //全时段投放
                    //                    ad.post_hours = 0xffffffffffff;
                    // //48个1
                    ad.set_post_hours(0xffffffffffff); // 48个1
                } else {
                    if (!root["post_hours"].isNull()) {
                        std::string post_hours;
                        post_hours = root["post_hours"].asString();
                        /*48*7=336, 一周7天，半个小时一个时段*/
                        if (post_hours.length() != 336) {
                            LOG_ERROR("post_hours error[%s]",
                                      post_hours.c_str());
                            rt = -1;
                            break;
                        }
                        struct tm tminfo;
                        util::Func::get_time_info(tminfo);
                        int weekday = tminfo.tm_wday;
                        int start_index = weekday * 48;
                        uint64_t hours = 0;
                        for (int n = 0; n < 48; n++) {
                            if (post_hours[start_index + n] == '1') {
                                hours |= (((uint64_t)1) << n);
                            }
                        }
                        ad.set_post_hours(hours);
                    } else {
                        ad.set_post_hours(0);
                    }
                }
            }

            Conj conj; // push一个空的进去
            vrconj.push_back(conj);
            int idx = 0;
            int conj_size = 0;
            std::vector<std::vector<TagType> > vrtagtype;
            if (!root["targeted_package"].isNull()) {
                std::string strtargeted = root["targeted_package"].asString();
                Json::Value tarroot;
                if (!reader_.parse(strtargeted, tarroot, false)) {
                    LOG_ERROR("json parse error");
                    rt = -1;
                    break;
                }
                Json::Value::Members mem = tarroot.getMemberNames();

                Json::Value::Members::iterator it;
                for (it = mem.begin(); it != mem.end(); it++) {

                    int type = atoi(it->c_str());
                    //城市ID不做为定向条件，因为分裂太厉害了
                    if (type == POST_REGION_TAG_ID) {
                        if (tarroot[*it].isArray() && tarroot[*it].size() > 0) {
                            int size = tarroot[*it].size();
                            for (int j = 0; j < size; j++) {
                                //                                maptar[type].push_back(tarroot[*it][j].asInt());
                                int value = tarroot[*it][j].asInt();
                                ad.add_post_region(value);
                            }
                        }
                        continue;
                    }

                    if (tarroot[*it].isArray() && tarroot[*it].size() > 0) {
                        std::vector<TagType> vrtag;
                        int size = tarroot[*it].size();
                        for (int j = 0; j < size; j++) {
                            //                            maptar[type].push_back(tarroot[*it][j].asInt());
                            int value = tarroot[*it][j].asInt();
                            TagType tag =
                                ((int64_t)type << 32) | (int64_t)value;
                            vrtag.push_back(tag);
                        }
                        vrtagtype.push_back(vrtag);
                        idx++;
                    }
                }
                conj_size = idx;
            }

            //把view_type 和source作为定向条件的一部分
            if (view_types.size() > 0) {
                std::vector<TagType> vrtag;
                std::set<int>::iterator it;
                for (it = view_types.begin(); it != view_types.end(); it++) {
                    int value = *it;
                    TagType tag =
                        ((int64_t)VIEW_TYPE_TARG_ID << 32) | (int64_t)value;
                    vrtag.push_back(tag);
                }
                vrtagtype.push_back(vrtag);
                conj_size++;
            }
            if (source != 0) {
                std::vector<TagType> vrtag;
                TagType tag = ((int64_t)SOURCE_TARG_ID << 32) | (int64_t)source;
                vrtag.push_back(tag);
                vrtagtype.push_back(vrtag);
                conj_size++;
            }
            if (!deal_id.empty()) {
                std::vector<TagType> vrtag;
                TagType tag = ((int64_t)DEAL_ID_TARG_ID << 32) |
                              util::Func::to_int(ad.pdb_data().deal_id());
                vrtag.push_back(tag);
                vrtagtype.push_back(vrtag);
                conj_size++;
            } else {
                //非PDB，加deal_id为0的定向
                std::vector<TagType> vrtag;
                TagType tag = ((int64_t)DEAL_ID_TARG_ID << 32) | 0;
                vrtag.push_back(tag);
                vrtagtype.push_back(vrtag);
                conj_size++;
            }

            for (idx = 0; idx < conj_size; idx++) {
                int tagsize = vrtagtype[idx].size();
                size_t conjsize = vrconj.size();
                for (int j = 0; j < tagsize; j++) {
                    if (j == tagsize - 1) {
                        //最后一个，在已有的基础上增加标签
                        for (size_t k = 0; k < conjsize; k++) {
                            vrconj[k].push_back(vrtagtype[idx][j]);
                        }
                    } else {
                        //同一个type有多个标签值，分裂出新的Conj出来
                        for (size_t k = 0; k < conjsize; k++) {
                            Conj newconj = vrconj[k];
                            newconj.push_back(vrtagtype[idx][j]);
                            vrconj.push_back(newconj);
                        }
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_ERROR("get exception[%s]buf[%s]\n", e.what(), buf);
        }
        catch (...)
        {
            LOG_ERROR("get unknowned exception");
        }
    } while (0);
    return rt;
}

int InvertedIndexBuilder::BuildInvertedIndex(const std::string &index_file)
{
    int rt = 0;
    FILE *fp = NULL;
    do {
        size_t max_conf_size = 0;
        fp = fopen(index_file.c_str(), "r");
        if (fp == NULL) {
            LOG_ERROR("fopen error,[%s]\n", index_file.c_str());
            rt = -1;
            break;
        }

        char buf[8192];
        //以下构建Conj-->Ad List
        std::map<int, std::set<Conj> > size_conj_set;
        std::map<Conj, std::vector<common::Ad> > map_conj_list;
        while (1) {
            memset(buf, 0x00, 8192);
            if (fgets(buf, 8192, fp) == NULL) {
                rt = false;
                break;
            }

            std::vector<Conj> vrconj;
            common::Ad ad;
            int nrt = ParseLine(buf, 8192, ad, vrconj);
            if (nrt != 0) {
                LOG_ERROR("ParseLine error[%s]", buf);
                rt = true;
                break;
            }
            std::vector<Conj>::iterator it;
            for (it = vrconj.begin(); it != vrconj.end(); it++) {
                if (it->size() > max_conf_size) {
                    max_conf_size = it->size();
                }
                std::sort(it->begin(), it->end());
                // conj->ad->ad...
                map_conj_list[*it].push_back(ad);
                // size->conj->conj...
                size_conj_set[it->size()].insert(*it);
            }
        }

        // size-->tag set
        std::map<int, std::set<TagType> > map_size_tag;

        // build:<size, tag>-->conj-->conj-->conj->...
        std::map<std::pair<int, TagType>, std::set<Conj> >
        map_size_tag_conj_set;

        std::map<int, std::set<Conj> >::iterator it;
        for (it = size_conj_set.begin(); it != size_conj_set.end(); ++it) {
            int size = it->first;
            std::set<Conj>::iterator itset;
            for (itset = it->second.begin(); itset != it->second.end();
                 ++itset) {
                // *itset:conj
                Conj::const_iterator it_tag;
                for (it_tag = itset->begin(); it_tag != itset->end();
                     ++it_tag) {
                    map_size_tag_conj_set[std::make_pair(size, *it_tag)]
                        .insert(*itset);
                    map_size_tag[size].insert(*it_tag);
                }

#if 0
                std::string conj_key = Conj2Str(*it);
                inverted_index::SizeTagValue size_tag_value;
                kvregapi_.put();
#endif
            }
        }

        // build data to kv sys <size,tag>->conj-list;
        //
        std::map<std::pair<int, TagType>, std::set<Conj> >::iterator itmap;
        for (itmap = map_size_tag_conj_set.begin();
             itmap != map_size_tag_conj_set.end(); ++itmap) {
            int size = itmap->first.first;
            TagType tag = itmap->first.second;
            std::set<Conj> &set_conj = itmap->second;
            util::UtilStr ustr;
            std::string key = ustr.format("%d_%llu_s_t", size, tag).str();

            inverted_index::SizeTagValue stvalue;
            stvalue.set_size(size);
            stvalue.set_tag(tag);
            std::set<Conj>::iterator itconj;
            for (itconj = set_conj.begin(); itconj != set_conj.end();
                 ++itconj) {
                //*itconj:Conj
                std::string conj_key = Conj2Str(*itconj);
                stvalue.add_conj_key(conj_key);
            }
            std::string value;
            if (!stvalue.SerializeToString(&value)) {
                LOG_ERROR("stvalue.SerializeToString error\n");
                continue;
            }

            int nrt = kvregapi_.put(key, value);
            if (nrt != 0) {
                LOG_ERROR("kvregapi_.put error[%d]\n", nrt);
            }
        }

        std::map<int, std::set<TagType> >::iterator it_m_s_t;
        for (it_m_s_t = map_size_tag.begin(); it_m_s_t != map_size_tag.end();
             ++it_m_s_t) {
            inverted_index::SizeTagList size_tag_list;
            int size = it_m_s_t->first;
            std::set<TagType> &set_tag = it_m_s_t->second;
            std::set<TagType>::iterator it_set_tag;
            for (it_set_tag = set_tag.begin(); it_set_tag != set_tag.end();
                 ++it_set_tag) {
                size_tag_list.add_tag(*it_set_tag);
            }
            util::UtilStr ustr;
            std::string key = ustr.format("s_t_l_%d", size).str();
            std::string value;
            if (!size_tag_list.SerializeToString(&value)) {
                LOG_ERROR("conj_info.SerializeToString error");
                continue;
            }
            int nrt = kvregapi_.put(key, value);
            if (nrt != 0) {
                LOG_ERROR("kvregapi_.put error[%d]\n", nrt);
                continue;
            }
        }

        // build data to kv sys conj_key->ad;
        std::map<Conj, std::vector<common::Ad> >::iterator it_map_conj;
        for (it_map_conj = map_conj_list.begin();
             it_map_conj != map_conj_list.end(); ++it_map_conj) {
            const Conj &conj = it_map_conj->first;
            std::vector<common::Ad> &vr_ad = it_map_conj->second;
            std::string key = Conj2Str(conj);
            inverted_index::ConjInfo conj_info;
            Conj::const_iterator it_tag;
            for (it_tag = conj.begin(); it_tag != conj.end(); ++it_tag) {
                conj_info.add_tag(*it_tag);
            }
            std::vector<common::Ad>::iterator itad;
            for (itad = vr_ad.begin(); itad != vr_ad.end(); ++itad) {
                common::Ad *pad = conj_info.add_ad();
                pad->CopyFrom(*itad);
            }
            std::string value;
            if (!conj_info.SerializeToString(&value)) {
                LOG_ERROR("conj_info.SerializeToString error");
                continue;
            }
            int nrt = kvregapi_.put(key, value);
            if (nrt != 0) {
                LOG_ERROR("kvregapi_.put error[%d]\n", nrt);
            }
        }
        // put max_conj_size
        std::string key = "max_conj_size";
        util::UtilStr ustr;
        std::string value = ustr.format("%d", max_conf_size).str();
        int nrt = kvregapi_.put(key, value);
        if (nrt != 0) {
            LOG_ERROR("kvregapi_.put error[%d]\n", nrt);
        }

        rt = kvregapi_.reg_data(INDEX_DATA_ID);
        if (rt < 0) {
            LOG_ERROR("kvregapi_.reg_data error[%d]\n", rt);
            break;
        }

    } while (0);
    if (fp != NULL) {
        fclose(fp);
    }
    return rt;
}
}
}
