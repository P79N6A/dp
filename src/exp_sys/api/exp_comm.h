#ifndef  _EXP_SYS_API_EXP_COMM_H_ 
#define  _EXP_SYS_API_EXP_COMM_H_

#define EXP_SHM_MARK 37648497

namespace poseidon
{
namespace exp_sys
{

enum PARAM_TYPE
{
    PT_INT=1,
    PT_FLOAT=2,
};

enum EXP_COMM
{
  EXP_SYS_DATA_ID=1,
};

struct ExpParam
{
    uint32_t param_id;   //参数ID
    PARAM_TYPE param_type; //参数类型
    union 
    {
        float float_v;
        int32_t int_v; 
    } param_vlaue;
};

struct MemHead
{
  uint32_t mark;
  uint32_t version;
  uint32_t last_update_shm_time;
  uint32_t mem_exp_info_num;
  uint32_t mem_exp_para_info_num;
  char md5[64];
};

struct MemExpInfo
{
  uint32_t mark;
  uint32_t exp_info_no;
  uint32_t module_id;
  uint32_t source_id;
  uint32_t view_type;
  uint32_t exp_id;
  uint32_t exp_valid_from;
  uint32_t exp_valid_to;
  uint32_t exp_quota_from;
  uint32_t exp_quota_to;
};

struct MemExpParaInfo
{
  uint32_t mark;
  uint32_t exp_para_info_no;
  uint32_t exp_id;
  ExpParam para;
};

}
}

#endif 