#!/bin/sh
extractor=$1
mert=$2
size=$3

if [ $# -ne 3 ]; then
    echo "Usage: ./normal_test.sh extracto mert size"
    exit 1
fi

echo "Runnning extractor ..."
$extractor --nbest NBEST --reference REF.0,REF.1,REF.2 --ffile FEATSTAT \
    --scfile SCORESTAT --sctype BLEU 2> extractor1.log

$extractor --ffile FEATSTAT.2 --scfile SCORESTAT.2 --sctype BLEU \
    --prev-ffile FEATSTAT --prev-scfile SCORESTAT 2> extractor2.log

$extractor --binary --ffile FEATSTAT.3 --scfile SCORESTAT.3 --sctype BLEU \
    --prev-ffile FEATSTAT,FEATSTAT.2 \
    --prev-scfile SCORESTAT,SCORESTAT.2 2> extractor3.log

$extractor --nbest NBEST --reference REF.0,REF.1,REF.2 --ffile FEATSTAT.4 \
    --scfile SCORESTAT.4 --sctype BLEU --prev-ffile FEATSTAT,FEATSTAT.3 \
    --prev-scfile SCORESTAT,SCORESTAT.3 2> extractor4.log

echo "Running mert ..."
$mert -r 1234 --ifile init.opt --scfile SCORESTAT --ffile FEATSTAT \
    -d $size --verbose 4 -n 5 2>mert.log

echo "Done."
