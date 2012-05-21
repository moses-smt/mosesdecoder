#!/bin/sh
extractor=$1
mert=$2
size=$3

if [ $# -ne 3 ]; then
    echo "Usage: ./normal_test.sh extracto mert size"
    exit 1
fi

if ! [ -f NBEST.gz ]; then
    gzip NBEST
fi

$extractor --nbest NBEST.gz --reference REF.0,REF.1,REF.2 \
    --ffile FEATSTAT_gz --scfile SCORESTAT_gz \
    --sctype BLEU 2> extractor_gz1.log

gzip -d NBEST.gz

$extractor --nbest NBEST --reference REF.0,REF.1,REF.2 \
    --prev-ffile FEATSTAT --prev-scfile SCORESTAT \
    --ffile FEATSTAT2 --scfile SCORESTAT2 \
    --sctype BLEU 2> extractor_gz2.log

# Now we want to test reading gzipped files.
# We will first compress the output previously.

for f in FEATSTAT_gz SCORESTAT_gz; do
    printf "Compressing %s " $f
    gzip $f
    echo "done."
done

$extractor --nbest NBEST --reference REF.0,REF.1,REF.2 \
    --prev-ffile FEATSTAT_gz.gz --prev-scfile SCORESTAT_gz.gz \
    --ffile FEATSTAT2_gz --scfile SCORESTAT2_gz \
    --sctype BLEU 2> extractor_gz3.log

gzip -d FEATSTAT_gz.gz SCORESTAT_gz.gz
echo "Done."
