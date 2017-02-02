#!/bin/sh
extractor=../extractor

VALGRIND_OPTS="--leak-check=full --leak-resolution=high \
--show-reachable=yes --track-origins=yes"
VALGRIND="valgrind $VALGRIND_OPTS"

for datafile in NBEST REF.0 REF.1 REF.2; do
    if ! [ -f $datafile ]; then
        echo "Error: $datafile does not exist."
        exit 1
    fi
done

$VALGRIND $extractor --nbest NBEST --reference REF.0,REF.1,REF.2 \
    --ffile FEATSTAT --scfile SCORESTAT --sctype BLEU
