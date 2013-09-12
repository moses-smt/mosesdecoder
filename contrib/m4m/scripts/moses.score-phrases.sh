#!/bin/bash 
# Wrapper script around the moses phrase scoring utility.
# Script by Ulrich Germann. Called from within M4M.
# 
# lexicon given should be
# de-given-en for fwd
# en-given-de for bwd

binary=$1
phrases=$2
lex=$3
obase=$4
smoothing=$5
inv=$6

cleanup()
{
    if [ -e $obase.$$ ] ;     then rm $obase.$$; fi
    if [ -e $obase.$$.coc ] ; then mv $obase.$$.coc $obase.coc; fi
}

mkfifo $obase.$$ || exit 1

trap 'cleanup' 0

export LC_ALL=C
if [[ "$inv" == "--Inverse" ]] ; then
    parallel < $obase.$$ -j10 --pipe --blocksize 250M "sort -S 10G | gzip > $obase.{#}.gz" &
else
    gzip < $obase.$$ > $obase.scored.gz_ &
fi

if [[ $phrases != "-" && $phrases != "/dev/stdin" ]] ; then
    $binary $phrases  <(zcat -f $lex) $obase.$$ $smoothing $inv || exit 1
else
    $binary /dev/stdin <(zcat -f $lex) $obase.$$ $smoothing $inv || exit 1
fi

if [ $? ] ; then exit $?; fi
wait
exit $?;
