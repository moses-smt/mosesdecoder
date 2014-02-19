#!/bin/bash
SD=`dirname $0`
export LD_LIBRARY_PATH=$SD/../../lib
java -Djava.library.path=$LD_LIBRARY_PATH -jar $SD/JniQueryPt.jar $1
