#!/bin/bash
SD=`dirname $0`
export LD_LIBRARY_PATH=$SD/../../lib
java -Djava.library.path=$LD_LIBRARY_PATH -cp $SD example $1
