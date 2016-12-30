#!/bin/bash

set -x
set -e

# 历史数据保留天数
N=5
# 历史数据路径
history_pb_data_path='/home/poseidon/ors_offline/budget_pacing/data/history'

last_day=$(date -d last-day +%Y%m%d)
expired_day=`date --date="${last_day} -${N} days" +%Y%m%d`

expired_pb_data_path=${history_pb_data_path}'/'${expired_day}
rm -rf ${expired_pb_data_path}

# 广告index数据路径
history_ads_index_data_path='/home/poseidon/ors_offline/budget_pacing/data'
# 广告index数据校验文件
adx_index_verify_file=${history_ads_index_data_path}'/index.data.done'

latest_ads_index_file=`cat ${adx_index_verify_file} | awk '{print $2}'`
latest_ads_index_date=${latest_ads_index_file:11:8}
ads_index_expired_day=`date --date="${latest_ads_index_date} -${N} days" +%Y%m%d`
ads_index_expired_day=`date --date="${ads_index_expired_day} -1 days" +%Y%m%d`
expired_ads_index_data_path=${history_ads_index_data_path}'/'index_date.${ads_index_expired_day}'*'
rm -f ${expired_ads_index_data_path}

expired_log_file='/home/poseidon/ors_offline/budget_pacing/log/ors_offline_budget_pacing.log.04'
rm -f ${expired_log_file}
