#!/bin/sh
#auth:zhangxh 20160817
#自动备份日志文件

#set despath=*log

source /etc/profile
source /home/poseidon/.bashrc

cd /home/poseidon/poseidon/bin

for i in $(ls *.log 2> /dev/null)
do
    if fuser $i &> /dev/null; then
      echo $i not zip! &> /dev/null
    else
      tar -zcv -f $i.tar.gz $i --remove-files &> /dev/null
    fi
done

mv *.tar.gz ./logs &> /dev/null


