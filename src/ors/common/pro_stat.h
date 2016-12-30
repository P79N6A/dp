/**
**/

#ifndef _ORS_PRO_STAT_H_
#define _ORS_PRO_STAT_H_

#include <set>
//include third_party_lib head files
#include "src/ors/common/shm_stat.h"

namespace poseidon
{
namespace ors
{

enum TPropertyId
{
    TPROPERTY_ID_ADX_REQ = 1,
    TPROPERTY_ID_ADX_RSP = 2,

    TPROPERTY_ID_POS_REQ = 11,
    TPROPERTY_ID_POS_RSP = 12,
    
    TPROPERTY_ID_ADVERTISER_REQ = 21,
    TPROPERTY_ID_ADVERTISER_RSP = 22,

    TPROPERTY_ID_CAMPAIGN_REQ = 31,
    TPROPERTY_ID_CAMPAIGN_RSP = 32,

    TPROPERTY_ID_AD_REQ = 41,
    TPROPERTY_ID_AD_RSP = 42,

    TPROPERTY_ID_CREATIVE_REQ = 51,
    TPROPERTY_ID_CREATIVE_RSP = 52,

    TPROPERTY_ID_DEAL_REQ = 61,
    TPROPERTY_ID_DEAL_RSP = 62,
};

class ProStat : public boost::serialization::singleton<ProStat>
{
public:
    ProStat()
    {
        m_stat_on = false;
        m_shm_ok = false;
    }
    ~ProStat()
    {

    }

    void PRO_ON()
    {
        m_stat_on = true;
        m_shm_ok = m_property_stat.InitShm();
    }

    void PRO_CLEAR()
    {
        pro_set.clear();
    }

    inline void PRO_STAT(int property_type, uint64_t property_id, int value)
    {
        if (m_stat_on && m_shm_ok)
        {
            std::pair<int ,uint64_t> p(property_type, property_id);
            if (pro_set.count(p) == 0) {
                m_property_stat.Add(property_type, property_id, value);
                pro_set.insert(p);
            }
        }
    }

    bool GetPropertyStat(const TPropertyKey& key, TPropertyValue** value)
    {
        return m_shm_ok && m_property_stat.Get(key, value);
    }

private:
    std::set< std::pair<int, uint64_t> > pro_set;
    bool m_stat_on;
    ShmStat m_property_stat;
    bool m_shm_ok;
};

#define PRO_ON() poseidon::ors::ProStat::get_mutable_instance().PRO_ON()
#define PRO_CLEAR() poseidon::ors::ProStat::get_mutable_instance().PRO_CLEAR()
#define PRO_STAT(property_type, property_id, val) poseidon::ors::ProStat::get_mutable_instance().PRO_STAT(property_type, property_id, val)
} // namespace ors
} // namespace poseidon

#endif // _ORS_PRO_STAT_H_

