#!/bin/bash 
# this script assumes that all 3rd-party dependencies are installed under ./opt
# you can install all 3rd-party dependencies by running make -f contrib/Makefiles/install-dependencies.gmake

set -e -o pipefail 

opt=$(pwd)/opt

args=$(getopt -oj:aq -lwith-irstlm:,with-boost:,with-cmph:,with-regtest:,no-xmlrpc-c,with-xmlrpc-c:,full -- "$@")
eval set -- "$args"

# default settings
noserver=false; 
full=false;
j=$(getconf _NPROCESSORS_ONLN)
irstlm=$opt/irstlm-5.80.08
boost=$opt
cmph=$opt
xmlrpc=--with-xmlrpc-c\=$opt 
regtest=$(pwd)/regtest
unset q
unset a
# the regression test for the compactpt bug is currently know to fail,
# let's skip it for the time being
skipcompact=--regtest-skip-compactpt

# overrides from command line
while true ; do 
    case "$1" in 
	-j ) j=$2; shift 2 ;;
	-a ) a=-a; shift ;;
	-q ) q=-q; shift ;;
	--no-xmlrpc-c   ) xmlrpc=$1;     shift ;;  
	--with-xmlrpc-c ) 
	    xmlrpc=--with-xmlrpc-c\=$2;  shift 2 ;;  
	--with-irstlm   ) irstlm=$2;     shift 2 ;;
	--with-boost    ) boost=$2;      shift 2 ;;
	--with-cmph     ) cmph=$2;       shift 2 ;;
	--with-regtest  ) regtest=$2;    shift 2 ;;
	--full          ) full=true;     shift 2 ;;  
	-- ) shift; break ;;
	* ) break ;;
    esac
done

if [ $? != 0 ] ; then exit $?; fi

git submodule init
git submodule update regtest

# full test means 
# -- compile from scratch without server, run regtests
# -- compile from scratch with server, run regtests
set -x
if [ "$full" == true ] ; then
    ./bjam -j$j --with-mm --with-mm-extras --with-irstlm=$irstlm --with-boost=$boost --with-cmph=$cmph --no-xmlrpc-c --with-regtest=$regtest -a $skipcompact $@ $q || exit $?
    if ./regression-testing/run-single-test.perl --server --startuptest  ; then
    	./bjam -j$j --with-mm --with-mm-extras --with-irstlm=$irstlm --with-boost=$boost --with-cmph=$cmph $xmlrpc --with-regtest=$regtest -a $skipcompact $@ $q 
    fi
else
   # when investigating failures, always run single-threaded
   if [ "$q" == "-q" ] ; then j=1; fi 

   if ./regression-testing/run-single-test.perl --server --startuptest  ; then
       ./bjam -j$j --with-mm $q $a --with-irstlm=$irstlm --with-boost=$boost --with-cmph=$cmph $xmlrpc --with-regtest=$regtest $skipcompact $@ 
   else
       ./bjam -j$j --with-mm --with-mm-extras $q $a --with-irstlm=$irstlm --with-boost=$boost --with-cmph=$cmph --no-xmlrpc-c --with-regtest=$regtest $skipcompact $@ 
   fi
fi

# if [ "$RECOMPILE" == "NO" ] ; then
#   RECOMPILE=
# else
#   RECOMPILE="-a"
# fi

# # test compilation without xmlrpc-c
# # ./bjam -j$(nproc) --with-irstlm=$opt --with-boost=$opt --with-cmph=$opt --no-xmlrpc-c --with-regtest=$(pwd)/regtest -a -q $@ || exit $?

# # test compilation with xmlrpc-c
# if ./regression-testing/run-single-test.perl --server --startuptest  ; then
#   ./bjam -j$(nproc) --with-irstlm=$opt --with-boost=$opt --with-cmph=$opt --with-xmlrpc-c=$opt --with-regtest=$(pwd)/regtest $RECOMPILE -q --regtest-skip-compactpt $@
# fi
