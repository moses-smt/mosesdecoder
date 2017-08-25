#!/bin/bash 
# if not supplied otherwise, this script assumes that all 3rd-party dependencies are installed under ./opt
# you can install all 3rd-party dependencies by running make -f contrib/Makefiles/install-dependencies.gmake

set -e -o pipefail
OPT=${OPT:-$(pwd)/opt}
./bjam --with-boost=$OPT --with-cmph=$OPT --with-xmlrpc-c=$OPT --with-mm --with-probing-pt -j$(getconf _NPROCESSORS_ONLN) $@

