#!/bin/bash
export LD_LIBRARY_PATH=../../lib
SD=`dirname $0`
java -Djava.library.path=$LD_LIBRARY_PATH -cp $SD/build example $1
