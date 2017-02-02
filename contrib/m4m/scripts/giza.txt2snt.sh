#!/bin/bash
# Wrapper script around plain2snt that allows us to generate the numberized
# files from gzipped text files via named pipes. (c) 2011-2012 Ulrich Germann

fail()
{
    echo $@
    exit 1
}

on_term()
{
    rm $odir/${L1}
    rm $odir/${L2}
}

trap 'on_term' TERM EXIT QUIT INT 0

if [ $# -lt 4 ]; then
    fail "usage: $0 <txtdir> <L1> <L2> <odir>"
fi

txtdir=$1
L1=$2
L2=$3
odir=$4

mkdir -p $odir
mkfifo $odir/${L1} || exit 1
mkfifo $odir/${L2} || exit 1

find -L ${txtdir} -name "*.${L1}" -or -name "*.${L1}.gz" | sort | xargs zcat -f > $odir/${L1} &
find -L ${txtdir} -name "*.${L2}" -or -name "*.${L2}.gz" | sort | xargs zcat -f > $odir/${L2} &

pushd $odir
plain2snt ${L1} ${L2}
wait
mv ${L1}_${L2}.snt ${L1}-${L2}.snt
mv ${L2}_${L1}.snt ${L2}-${L1}.snt
wait
popd
