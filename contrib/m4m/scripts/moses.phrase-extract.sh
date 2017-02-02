#!/bin/bash
# Helper script for phrase extraction from a single corpus shard.
# Written by Ulrich Germann.

# to be added: built-in factor filtering for factored models

cleanup()
{
    if [ -e $fifo ] ;     then rm $fifo;     fi
    if [ -e $fifo.inv ] ; then rm $fifo.inv; fi
    if [ -e $fifo.o ] ;   then rm $fifo.o;  fi
}

usage()
{
    echo
    echo "$0: wrapper script to extract phrases from word-aligned corpus"
    echo -e "usage:\n   $0 <extractor> <ibase> <L1tag> <L2tag> [-x] "
    echo "options:"
    echo "-l: maximum phrase length ($plen)"
    echo "-m: distortion model specification"
    echo "-o: base name for output files .fwd.gz .bwd.gz [.<dmodel>.dst.gz]"
    echo "-x: (no argument) don't create .fwd.gz and .bwd.gz"
    echo
    echo "required input files:  <ibase>.<L1tag>.gz ibase.<L2tag>.gz ibase.<aln>.gz"
}

plen=7
nottable=
dmodel=
dspec=
pargs=
sfactors=
tfactors=
while [ $# -gt 0 ]; do
    case $1 in
	-l*) plen=${1#-l}
	    plen=${plen#=}
	    if [ -z $plen ] ; then 
		shift
		plen=$1
	    fi
	    ;;
	-m*) dmodel=${1#-m}
	    dmodel=${dmodel#=}
	    if [ -z $dmodel ] ; then
		shift
		dmodel="$1"
	    fi
	    ;;
	-o*) obase=${1#-o}
	    obase=${obase#=}
	    if [ -z $obase ] ; then
		shift
		obase=$1
	    fi
	    ;;
	-s*) sfactors=${1#-s}
	    sfactors=${sfactors#=}
	    if [ -z $sfactors ] ; then
		shift
		sfactors = $1
	    fi
	    ;;
	-t*) tfactors=${1#-t}
	    tfactors=${tfactors#=}
	    if [ -z $tfactors ] ; then
		shift
		sfactors = $1
	    fi
	    ;;
	-x) nottable=1;;
	-h) usage; exit 0;;
	*) pargs=(${pargs[*]} $1);;
    esac
    shift
done

if [ -n "$sfactors" ] || [ -n "$tfactors" ] ; then
    echo "Factor filtering is not implemented yet!"
    exit 2
fi

extract=${pargs[0]}
ibase=${pargs[1]}
L1tag=${pargs[2]}
L2tag=${pargs[3]}
obase=${obase:=$ibase}

fifo=$obase.$$
trap 'cleanup' 0

export LC_ALL=C
if [ -z "$nottable" ] ; then 
mkfifo $fifo;     sort -S 5G < $fifo     | gzip > $obase.fwd.gz &
mkfifo $fifo.inv; sort -S 5G < $fifo.inv | gzip > $obase.bwd.gz &
fi
if [ -n "$dmodel" ] ; then
    mkfifo $fifo.o 
    sort -S 5G < $fifo.o | gzip > $obase.dst.gz &
    dspec="orientation --model "
    dspec+=`echo $dmodel | perl -pe 's/((hier|phrase|wbe)-(msd|msrl|mono)).*/$1/;'`
fi

txt1=${ibase}.${L1tag}.gz
txt2=${ibase}.${L2tag}.gz
aln=${ibase}.aln.gz
echo "($extract <(zcat -f $txt1) <(zcat -f $txt2) <(zcat -f $aln) $fifo $plen $dspec) || exit 1"
($extract <(zcat -f $txt2) <(zcat -f $txt1) <(zcat -f $aln) $fifo $plen $dspec) || exit 1
wait
