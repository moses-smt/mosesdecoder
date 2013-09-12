#!/bin/bash
# helper script for phrase extraction
# (c) 2011-2012 Ulrich Germann
# txtdir - directory with gzipped plain text files
# sntdir - directory with files in Giza's .snt format, also including the .OK files
#          produced by giza.txt2snt.sh
# gizdir - directory where aligned corpus resides
# L1,L2  - language tags for L1,L2
# plmax  - max phrase length to be extraced

extractor=$1
L1_text=$2
L2_text=$3
aln=$4
odir=$5
max_plen=$6
dmodel=$7


echo $#
if [ $# -lt 6 ] ; then 
    echo <<EOF \
"usage: $0 <moses-extract-command> <L1 text> <L2 text> <alignment file> <output dir> <max phrase length> <distortion-model>"
EOF
exit 1
fi

fifo=$odir/fifo.$$

cleanup()
{
    if [ -e $fifo ] ;     then rm $fifo;     fi
    if [ -e $fifo.inv ] ; then rm $fifo.inv; fi
    if [ -e $fifo.o ] ;   then rm $fifo.o;  fi
}

trap 'cleanup' 0
export LC_ALL=C
mkdir -p $odir/fwd $odir/bwd $odir/dst
mkfifo $fifo 
parallel < $fifo -j6 --pipe --blocksize 250M "sort -S 5G | gzip > $odir/fwd/part.{#}.gz" &
mkfifo $fifo.inv 
parallel < $fifo.inv -j6 --pipe --blocksize 250M "sort -S 5G | gzip > $odir/bwd/part.{#}.gz" &
if [ "$dmodel" != "" ] ; then
    mkfifo $fifo.o 
    parallel < $fifo.o -j6 --pipe --blocksize 250M "sort -S 5G | gzip > $odir/dst/part.{#}.gz" &
    dmodel="orientation --model $dmodel" 
fi
#echo "($extractor <(zcat -f $L2_text) <(zcat -f $L1_text) <(zcat -f $aln) $fifo $max_plen $dmodel) || exit 1"
($extractor <(zcat -f $L2_text) <(zcat -f $L1_text) <(zcat -f $aln) $fifo $max_plen $dmodel) || exit 1

wait

# for part in fwd bwd dst; do 
#     echo -n '' > $odir/${part}/sort.batch
#     for f in $odir/${part}/part.[0-9][0-9][0-9][0-9].gz; do
#         g=`echo $f | sed 's/.gz$//'`
# #        echo "f=$g; if [ -e \$f.gz ] ; then zcat \$f.gz | LC_ALL=C sort | gzip > \$f.gz_ && mv \$f.gz_ \$f.sorted.gz && rm \$f.gz; fi" \
#         echo "f=$g; if [ -e \$f.gz ] ; then zcat \$f.gz | LC_ALL=C sort | gzip > \$f.gz_ && mv \$f.gz_ \$f.sorted.gz; fi" \
#             >> $odir/${part}/sort.batch
#     done
# done

