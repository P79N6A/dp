#!/bin/sh
source /etc/profile
source /home/xianghuizhang/.bashrc

cd /home/xianghuizhang/work/Poseidon/src/adapter

#增加monitor.sh命令的grpe忽略
ps -fe|grep poseidon_adapter|grep -v grep|grep -v monitor.sh
if [ $? -ne 0 ]
then
echo "No process exists. Start process....."
run.sh
else
echo "Runing....."
fi
