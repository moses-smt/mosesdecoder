#!/bin/bash

git submodule init
git submodule update regtest
while [ true ] ; do 
    echo 'test' | bin/moses -f test.ini && break
    commit=$(git log | grep ^commit | head -n2 | tail -n1 | sed 's/commit //')
    echo REVERTING TO COMMIT $commit
    git checkout $commit
    ./bjam -j$(nproc) --with-boost=$(pwd)/opt --with-mm -a -q  
done
