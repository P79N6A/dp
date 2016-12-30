/**
**/

#ifndef _ORS_PDB_INFO_H_
#define _ORS_PDB_INFO_H_
//include STD C/C++ head files


//include third_party_lib head files
#include "protocol/src/poseidon_proto.h"

namespace poseidon
{
namespace ors
{
class PdbInfo
{

public:
    PdbInfo()
    {
    }
    virtual ~PdbInfo()
    {
    }
    virtual bool Bind(const AlgoRequest& algo_request);
    bool IsValid()
    {
        return m_deal_id.size() > 0;
    }

    const std::string& GetDealId()
    {
        return m_deal_id;
    }

    float GetDealPrice()
    {
        return m_deal_price;
    }


    float GetCampaignAdQuato(uint32_t campaign_id)
    {
        if (m_campaign_quato.find(campaign_id) != m_campaign_quato.end())
        {
            return m_campaign_quato[campaign_id] / m_campaign_ad_num[campaign_id];
        }

        return 1.0f;
    }

    float GetFillingRate()
    {
        return m_fill_rate;
    }

    uint64_t GetHashDealId()
    {
        return m_hash_deal_id;
    }

protected:
    std::string m_deal_id;
    uint64_t m_hash_deal_id;
    float m_deal_price;
    float m_fill_rate;
    std::map<uint32_t, uint32_t>  m_campaign_ad_num;
    std::map<uint32_t, float> m_campaign_quato;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_PDB_INFO_H_

