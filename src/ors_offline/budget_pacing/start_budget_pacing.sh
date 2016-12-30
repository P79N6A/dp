#!/bin/sh

set -ex

config='./conf/budget_pacing.conf'

if [ -d log ]; then
    mkdir log
    echo "create dir log"
fi

if [ -f ${config} ]; then
	echo "Budget_pacing start.\n"
	./budget_pacing ${config}
else
	echo "Do not found config file:${config}."
fi
