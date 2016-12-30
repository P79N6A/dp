/**
**/

#ifndef _ORS_QUERY_ACCESSOR_H_
#define _ORS_QUERY_ACCESSOR_H_
//include STD C/C++ head files
#include <vector>

//include third_party_lib head files
#include "protocol/src/poseidon_proto.h"
#include "src/ors/common/common_def.h"
#include "src/ors/common/video_info.h"
#include "src/ors/common/pdb_info.h"

namespace poseidon
{
namespace ors
{
class QueryAccessor
{

public:
    QueryAccessor();
    virtual ~QueryAccessor();
    virtual int Bind(const AlgoRequest& algo_request);

    uint32_t GetAdxId()
    {
        return m_adx_id;
    }


    const std::string& GetPosId()
    {
        return m_pos_id;
    }

    uint64_t GetPosHashId()
    {
        return m_pos_hash_id;
    }

    const std::string& GetDomain()
    {
        return m_domain;
    }

    const std::string& GetSite()
    {
        return m_site;
    }

    const std::string& GetUrl()
    {
        return m_url;
    }

    uint32_t GetReqAdNum()
    {
        return m_req_ad_num;
    }

    const std::vector<std::string>& GetKeywords()
    {
        return m_keywords;
    }

    const std::vector<int>& GetSiteCategories()
    {
        return m_site_categories;
    }

    float GetFloorPrice()
    {
        return m_floor_price;
    }

    const std::string& GetDeviceId()
    {
        return m_device_id;
    }

    const std::string& GetOs()
    {
        return m_os;
    }

    uint32_t GetOsType()
    {
        return m_os_type;
    }
   
    bool HasUserTag(int type)
    {  
        std::map<int, int>::iterator iter = m_user_tags.find(type);
        if (iter != m_user_tags.end())
        {
            return true;
        }
        return false;
    }

    bool GetUserTagValue(int type, int* value)
    {
        *value = 0;
        std::map<int, int>::iterator iter = m_user_tags.find(type);
        if (iter != m_user_tags.end()) 
        {
            *value = iter->second;
            return true;
        }
        return false;
    }

    void SetTrace(int fid, int fval)
    {
        m_traces[fid].u.fixed = fval;
    }

    void SetTrace(int fid, float fval)
    {
        m_traces[fid].u.real = fval;
    }

    int GetTraceValueInterger(int fid)
    {
        return m_traces[fid].u.fixed;
    }

    float GetTraceValueFloat(int fid)
    {
        return m_traces[fid].u.real;
    }

    FValue GetTraceValue(int fid)
    {
        return m_traces[fid];
    }

    AdxBaseParam* GetAdxBaseParam()
    {
        return &m_adx_base_param;
    }

    PosBaseParam* GetPosBaseParam()
    {
        return &m_pos_base_param;
    }

    VideoInfo* GetVideoInfo()
    {   
        return &m_video_info;
    }

    bool IsVideo()
    {
        return m_video_info.IsValid();
    }

    bool IsPdb()
    {
        return m_pdb_info.IsValid();
    }

    uint32_t GetViewType()
    {
        return m_view_type;
    }

    PdbInfo* GetPdbInfo()
    {
        return &m_pdb_info;
    }

    const std::string& GetAppName()
    {
        return m_app_name;
    }

    uint64_t GetAppHashId()
    {
        return m_app_hash_id;
    }

    uint32_t GetPosSize()
    {
        return m_width*1000 + m_height;
    }

    bool GetExpParam(int exp_param_id, int* exp_param_val)
    {
        std::map<int, int>::iterator iter = m_int_exp_params.find(exp_param_id);
        if (iter != m_int_exp_params.end())
        {
            *exp_param_val = iter->second;
            return true;
        }
        return false;
    }

    bool GetExpParam(int exp_param_id, float* exp_param_val)
    {
        std::map<int, float>::iterator iter = m_float_exp_params.find(exp_param_id);
        if (iter != m_float_exp_params.end())
        {
            *exp_param_val = iter->second;
            return true;
        }
        return false;
    }


protected:
    uint32_t m_adx_id;
    std::string m_pos_id;
    uint32_t m_view_type;
    uint32_t m_width;
    uint32_t m_height;
    std::string m_app_name;
    uint64_t m_pos_hash_id;
    uint64_t m_app_hash_id;
    std::string m_domain;
    std::string m_site;
    std::string m_url;
    std::vector<int> m_site_categories;
    std::vector<std::string> m_keywords;
    uint32_t m_req_ad_num;
    float m_floor_price;
   
    std::string m_device_id;
    std::string m_os;
    uint32_t m_os_type;
    std::map<int, int> m_user_tags;
    std::set<int> m_seed_user_types;

    FValue m_traces[T_ID_MAX_NUM];
    AdxBaseParam m_adx_base_param;
    PosBaseParam m_pos_base_param;
    VideoInfo m_video_info;

    PdbInfo m_pdb_info;
    std::map<int, int> m_int_exp_params;
    std::map<int, float> m_float_exp_params;
};
} // namespace ors
} // namespace poseidon

#endif // _ORS_XXX_H_

