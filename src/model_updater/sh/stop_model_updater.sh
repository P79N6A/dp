#!/bin/sh

set -e
procs=`ps aux | grep poseidon_model_updater | grep conf |awk '{print $2}'`
for proc in $procs
do
    kill $proc
    echo "kill $proc ok"
done 
proc_num=`ps aux | grep poseidon_model_updater | grep conf|wc -l`
if [ $proc_num -gt 0 ];then
    for proc in $procs
    do
        kill -9 $proc
        echo "FUCK kill -9 $proc"
    done
fi

exit 0
