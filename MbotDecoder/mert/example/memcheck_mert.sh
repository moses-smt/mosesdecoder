#!/bin/sh
VALGRIND_OPTS="--leak-check=full --leak-resolution=high \
--show-reachable=yes --track-origins=yes"
VALGRIND="valgrind $VALGRIND_OPTS"

size=15
extractor=../extractor
mert=../mert

# Make sure you need to run extractor.
for f in SCORESTAT FEATSTAT; do
    if ! [ -f "$f" ]; then
        echo "Error: $f does not exist. Now we are running extractor."
        $extractor --nbest NBEST --reference REF.0,REF.1,REF.2 \
            --ffile FEATSTAT --scfile SCORESTAT --sctype BLEU
        break
    fi
done

$VALGRIND $mert -r 1234 --ifile init.opt --scfile SCORESTAT \
    --ffile FEATSTAT -d $size --verbose 4 -n 5
