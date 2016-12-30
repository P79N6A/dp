#!/bin/sh
ps aux | grep poseidon_model_updater| grep conf
proc_num=`ps aux | grep poseidon_model_updater| grep conf |wc -l`
echo "poseidon_model_updater has ${proc_num} process"
exit ${proc_num} 
