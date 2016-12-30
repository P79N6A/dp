/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/pdb_info.h"
#include "util/log.h"
#include "util/func.h"

namespace poseidon
{
namespace ors
{

bool PdbInfo::Bind(const AlgoRequest& algo_request)
{
    m_deal_id = "";
    m_deal_price = 0.01f;
    m_fill_rate = 0.0f;
    m_campaign_ad_num.clear();
    m_campaign_quato.clear();
    for (int i = 0; i < algo_request.algo_ads_size(); i++)
    {
        const common::Ad& ad = algo_request.algo_ads(i).ad();
        if (ad.has_pdb_data())
        {
            const common::Ad_PdbData& pdb_data = algo_request.algo_ads(i).ad().pdb_data();
            if (pdb_data.has_deal_id())
            {
                m_deal_id = pdb_data.deal_id();
                m_deal_price = pdb_data.settle_price() / 100.0f;
                m_fill_rate = pdb_data.fill_rate() /100.0f;
            
                if (m_campaign_ad_num.find(ad.campaign_id()) == m_campaign_ad_num.end())
                {
                    m_campaign_ad_num[ad.campaign_id()] = 0;
                }
                m_campaign_ad_num[ad.campaign_id()] += 1;
                m_campaign_quato[ad.campaign_id()] = pdb_data.campaign_quota() / 1000.0f;
            }
        }
    }

    for (std::map<uint32_t, uint32_t>::iterator iter = m_campaign_ad_num.begin(); iter != m_campaign_ad_num.end(); iter++)
    {
        LOG_DEBUG("pdb campaign_id=%d, ad_num=%d, quato = %f", iter->first, iter->second, m_campaign_quato[iter->first]);
    }

    if (m_deal_id.length() > 0)
    {
        m_hash_deal_id = atoi(m_deal_id.c_str());
        if (m_hash_deal_id == 0)
        {
             m_hash_deal_id = util::Func::BytesHash64(m_deal_id.c_str(), m_deal_id.length());
        }
    }

    LOG_DEBUG("PdbInfo Bind OK!deal_id=%s, deal_price=%f, fill_rate=%f", m_deal_id.c_str(), m_deal_price, m_fill_rate);    
    return true;

}

} // namespace ors
} // namespace poseidon

