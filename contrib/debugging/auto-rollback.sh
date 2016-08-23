#!/bin/bash

git submodule init
git submodule update regtest
while [ true ] ; do 
    ./bjam -j$(nproc) --with-irstlm=$(pwd)/opt --with-boost=$(pwd)/opt --with-cmph=$(pwd)/opt --with-xmlrpc-c=$(pwd)/opt --with-regtest=$(pwc)/regtest -a -q $@ && break
    commit=$(git log | grep ^commit | head -n2 | tail -n1 | sed 's/commit //')
    echo REVERTING TO COMMIT $commit
    git checkout $commit
done
