#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ -e ${DIR}/server.pid ]
then
        kill -9 $(cat ${DIR}/server.pid)
        rm server.pid
fi

if [ -e ${DIR}/client.pid ]
then
        kill -9 $(cat ${DIR}/client.pid)
        rm client.pid
fi
