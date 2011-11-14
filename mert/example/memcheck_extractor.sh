#!/bin/sh
extractor=../extractor

VALGRIND_OPTS="--leak-check=full --leak-resolution=high \
--show-reachable=yes --track-origins=yes"
VALGRIND="valgrind $VALGRIND_OPTS"
$VALGRIND $extractor --nbest NBEST --reference REF.0,REF.1,REF.2 \
    --ffile FEATSTAT --scfile SCORESTAT --sctype BLEU
