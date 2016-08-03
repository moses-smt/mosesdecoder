#!/bin/bash


SRC=$1
TRG=$2

FSRC=all_${1}-${2}.${1}
FTRG=all_${1}-${2}.${2}

echo "" > $FSRC
for F in *${1}-${2}.${1}
do
    if [ "$F" = "$FSRC" ]; then
        echo "pass"
    else
        cat $F >> $FSRC
    fi
done


echo "" > $FTRG
for F in *${1}-${2}.${2}
do
    if [ "$F" = "$FTRG" ]; then
        echo "pass"
    else
        cat $F >> $FTRG
    fi
done
