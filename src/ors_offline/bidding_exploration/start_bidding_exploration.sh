#!/bin/sh

cd $(dirname $0)

config='./conf/bidding_exploration.conf'

if [ -f ${config} ]; then
	echo "bidding_exploration beginning.\n"
	./bidding_exploration ${config}
else
	echo "Do not found config file:${config}."
fi
