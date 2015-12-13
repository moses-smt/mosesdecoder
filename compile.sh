#!/bin/bash 
# this script assumes that all 3rd-party dependencies are installed under ./opt
# you can install all 3rd-party dependencies by running make -f contrib/Makefiles/install-dependencies.gmake

set -e -o pipefail
opt=$(pwd)/opt
./bjam --with-irstlm=$opt --with-boost=$opt --with-cmph=$opt --with-xmlrpc-c=$opt --with-mm --with-probing-pt -j$(getconf _NPROCESSORS_ONLN) $@

