#!/bin/sh

cd $(dirname $0)

sh_pid=`ps aux | grep "./start_bidding_exploration.sh" | grep -v "grep" | awk '{print $2}'`

if [ -n "${sh_pid}" ]; then
	echo "Stop start_bidding_exploration.sh shell script(pid="${sh_pid}")."
	kill -9 ${sh_pid}
fi

bin_pid=`ps aux | grep "./bidding_exploration" | grep -v "grep" | awk '{print $2}'`

if [ -n "${bin_pid}" ]; then
	echo "Stop bidding_exploration(pid="${bin_pid}")."
	kill -9 ${bin_pid}
fi
