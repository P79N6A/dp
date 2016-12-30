#!/bin/bash

set -x
set -e

# 历史数据保留天数
N=5
# 历史数据路径
history_data_path='/home/poseidon/ors_offline/bidding_exploration/data/history'

last_day=$(date -d last-day +%Y%m%d)
expired_day=`date --date="${last_day} -${N} days" +%Y%m%d`

expired_data_path=${history_data_path}'/'${expired_day}
rm -rf ${expired_data_path}

expired_log_file='/home/poseidon/ors_offline/bidding_exploration/log/ors_offline_bidding_exploration.
log.04'
rm -f ${expired_log_file}
