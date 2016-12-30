#!/bin/sh

sh_pid=`ps aux | grep "./start_budget_pacing.sh" | grep -v "grep" | awk '{print $2}'`

if [ -n "${sh_pid}" ]; then
	echo "Stop start_budget_pacing.sh shell script(pid="${sh_pid}")."
	kill -9 ${sh_pid}
fi

bin_pid=`ps aux | grep "./budget_pacing" | grep -v "grep" | awk '{print $2}'`

if [ -n "${bin_pid}" ]; then
	echo "Stop budget_pacing(pid="${bin_pid}")."
	kill -9 ${bin_pid}
fi
