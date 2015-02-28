#!/bin/bash

default=ASST2

if [ $# -eq 0 ]
then
	asst=$default
	echo $asst
else
	asst=$1
fi

cd /home/trinity/src/kern/conf
./config $asst
cd ../compile/$asst
bmake depend
bmake
bmake install