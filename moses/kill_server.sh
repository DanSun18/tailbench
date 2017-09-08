#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -e ${DIR}/server.pid ]
then
	kill $(cat ${DIR}/server.pid)
	rm ${DIR}/server.pid
fi
