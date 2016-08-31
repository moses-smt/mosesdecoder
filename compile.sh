#!/bin/bash 
# if not supplied otherwise, this script assumes that all 3rd-party dependencies are installed under ./opt
# you can install all 3rd-party dependencies by running make -f contrib/Makefiles/install-dependencies.gmake

set -e -o pipefail
opt=$(pwd)/opt
irstlm=${opt}/irstlm-5.80.08
./bjam --with-boost=$opt --with-cmph=$opt --with-xmlrpc-c=$opt --with-mm --with-probing-pt -j$(getconf _NPROCESSORS_ONLN) --with-vw=$opt/vw $@

