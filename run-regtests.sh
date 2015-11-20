#!/bin/bash 
# this script assumes that all 3rd-party dependencies are installed under ./opt
# you can install all 3rd-party dependencies by running make -f contrib/Makefiles/install-dependencies.gmake

set -e -o pipefail -x

opt=$(pwd)/opt
git submodule init
git submodule update regtest
if [ "$RECOMPILE" == "NO" ] ; then
  RECOMPILE=
else
  RECOMPILE="-a"
fi

# test compilation without xmlrpc-c
./bjam -j$(nproc) --with-irstlm=$opt --with-boost=$opt --with-cmph=$opt --no-xmlrpc-c --with-regtest=$(pwd)/regtest -a -q $@ || exit $?

# test compilation with xmlrpc-c
if ./regression-testing/run-single-test.perl --server --startuptest  ; then
  ./bjam -j$(nproc) --with-irstlm=$opt --with-boost=$opt --with-cmph=$opt --with-xmlrpc-c=$opt --with-regtest=$(pwd)/regtest $RECOMPILE -q $@
fi
