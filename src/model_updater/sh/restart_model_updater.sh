#!/bin/sh

ShPath=`dirname $(readlink -f $0)`
WorkPath=`dirname $ShPath`

cd ${WorkPath}/sh
./stop_model_updater.sh
sleep 1
cd ${WorkPath}/bin
./poseidon_model_updater --conf=../etc/conf.cfg >> ../log/stderr.log 2>&1
ret=$?
if [ $ret -eq 0 ];then
    echo "YES LOVE YOU!!"
    echo "poseidon_model_updater Start OK!"
    exit 0
fi

echo "NO FUCK YOU!!"
echo "poseidon_model_updater Start Failed!"
exit 1
