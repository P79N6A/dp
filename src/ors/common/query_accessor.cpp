/**
**/

//include STD C/C++ head files


//include third_party_lib head files
#include "src/ors/common/query_accessor.h"
#include "util/log.h"
#include "util/func.h"
#include "src/ors/common/pro_stat.h"

namespace poseidon
{
namespace ors
{

QueryAccessor::QueryAccessor()
{

}

QueryAccessor::~QueryAccessor()
{

}

int QueryAccessor::Bind(const AlgoRequest& algo_request)
{
    for (int i = 0; i < T_ID_MAX_NUM; i++)
    {
        m_traces[i].SetNull();
    }
    this->SetTrace(T_ID_BIDDING_EXPLORE_FLAG, 0);
    this->SetTrace(T_ID_CONTEXT_GRADE_QUALITY, -1);

    const TrafficInfo& traffic_info = algo_request.traffic_info();
    m_adx_id = traffic_info.traffic_source();
    m_pos_id = traffic_info.imp_id();
    m_view_type = traffic_info.view_type();
    if (m_adx_id == ADX_ID_TOUTIAO)
    {
        m_pos_id = "tt1"; // hardcode
        //m_pos_id = util::Func::to_str(traffic_info.view_type());
    }
    m_pos_hash_id = atoll(m_pos_id.c_str());
    if (m_pos_hash_id == 0) {
        m_pos_hash_id = util::Func::BytesHash64(m_pos_id.data(), m_pos_id.length());
    }
    m_app_name = traffic_info.app_name();
    m_app_hash_id = 0;
    if (!m_app_name.empty())
    {
        m_app_hash_id = util::Func::BytesHash64(m_app_name.data(), m_app_name.length());
    }
    m_floor_price = traffic_info.min_cpm_price()/100.0f;
    m_req_ad_num = traffic_info.ad_num();
    m_domain = traffic_info.domain();
    m_site = traffic_info.site();
    m_width = traffic_info.width();
    m_height = traffic_info.height();

    m_keywords.resize(traffic_info.keywords_size());
    for (int i = 0; i < traffic_info.keywords_size(); i++)
    {
        m_keywords[i] = traffic_info.keywords(i);
    }

    m_site_categories.resize(traffic_info.site_categories_size());
    for (int i = 0; i < traffic_info.site_categories_size(); i++)
    {
        m_site_categories[i] = traffic_info.site_categories(i);
    }

    m_user_tags.clear();
    m_seed_user_types.clear();
    for (int i = 0; i < algo_request.targets_size(); i++)
    {
        const common::Targetting& targetting = algo_request.targets(i);
        int type =  targetting.type();

        if (type == 5005) {
            for (int j = 0; j < targetting.value_size(); j++) {
                int val = util::Func::to_int(targetting.value(j));
                if (val >= 11 && val <= 18) {
                    this->SetTrace(T_ID_USER_SEED_ID_PAY, val);
                    LOG_DEBUG("pay user_seed_id = %d", val);
                } else if (val >= 21 && val <= 28) {
                    this->SetTrace(T_ID_USER_SEED_ID_ACTIVE, val);
                    LOG_DEBUG("active user_seed_id = %d", val);
                }
            }
        }

        if (type == USER_TAG_TYPE_SEED_USER)
        {
            if (targetting.value_size() > 1)
            {
                for (int j = 1; j < targetting.value_size(); j += 2)
                {
                    int val = util::Func::to_int(targetting.value(j));
                    m_seed_user_types.insert(val);
                    LOG_DEBUG("User Tags type=%d, value=%s", type, targetting.value(j).c_str());
                }

                for (std::set<int>::iterator iter = m_seed_user_types.begin(); iter != m_seed_user_types.end(); iter++) {
                    if (*iter == 1001) {
                        this->SetTrace(T_ID_USER_SEED_CATE_PAY, *iter);
                        LOG_DEBUG("seed user cate pay = %d", *iter);
                    }
                    if (*iter >= 1101 && *iter <= 1103) {
                         this->SetTrace(T_ID_USER_SEED_CATE_ACTIVE, *iter);
                         LOG_DEBUG("seed user cate active = %d", *iter);
                         break;
                    }
                }
            }
        }
        else 
        {
            //todo  now only get the first type value
            for (int j = 0; j < targetting.value_size() && j < 1; j++)
            {
                m_user_tags[type] = util::Func::to_int(targetting.value(j));
                LOG_DEBUG("User Tags type=%d, value=%s", type, targetting.value(j).c_str());
            }
        }
    }

    m_device_id = algo_request.device_info().id();
    m_os = algo_request.device_info().os();
    m_os_type = 0;
    if (m_os == "android") {
        m_os_type = OS_TYPE_ANDROID;
    } else if (m_os == "ios") {
        m_os_type = OS_TYPE_IOS;
    }

    if (!m_video_info.Bind(m_adx_id, algo_request.video()))
    {
        LOG_ERROR("VideoInfo Bind Failed!");
        return false;
    }

    if (!m_pdb_info.Bind(algo_request))
    {
        LOG_ERROR("PdbInfo Bind Failed!");
        return false;
    }

    if (m_pdb_info.IsValid())
    {
        PRO_STAT(TPROPERTY_ID_DEAL_REQ, m_pdb_info.GetHashDealId(), m_req_ad_num);
    }

    m_adx_base_param.Clear();
    m_pos_base_param.Clear();

    m_int_exp_params.clear();
    m_float_exp_params.clear();

    // 读取实验参数
    for (int i = 0; i < algo_request.exp_param_size(); i++)
    {
        if (algo_request.exp_param(i).has_int_value())
        {
            m_int_exp_params[algo_request.exp_param(i).param_id()] = algo_request.exp_param(i).int_value();
            LOG_DEBUG("ExpParam param_id = %d, int_value = %d", algo_request.exp_param(i).param_id(), algo_request.exp_param(i).int_value());
        }
        if (algo_request.exp_param(i).has_float_value())
        {
            m_float_exp_params[algo_request.exp_param(i).param_id()] = algo_request.exp_param(i).float_value();
            LOG_DEBUG("ExpParam param_id = %d, float_value = %d", algo_request.exp_param(i).param_id(), algo_request.exp_param(i).float_value());
        }
    }
    
    PRO_STAT(TPROPERTY_ID_ADX_REQ, m_adx_id, m_req_ad_num);
    PRO_STAT(TPROPERTY_ID_POS_REQ, m_pos_hash_id, m_req_ad_num);
    LOG_DEBUG("Bind OK!adx_id=%d, pos_id=%s, pos_hash_id=%lu, view_type=%d, floor_price=%f, size=%u,req_ad_num=%d", 
            m_adx_id, m_pos_id.c_str(), m_pos_hash_id, m_view_type, m_floor_price, this->GetPosSize(), m_req_ad_num);
    return 0;
}


} // namespace ors
} // namespace poseidon

