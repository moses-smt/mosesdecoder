#!/bin/sh

PATH=$PATH:/fs/hel1/nadir/SRILM/bin/i686-m64/

echo 'Training OSM - Start'
date

\rm $5/e
\rm $5/f
\rm $5/align

ln -s $1 $5/e
ln -s $2 $5/f

./flipAlignment $3 > $5/align

echo 'Extracting Singletons'

./extract-singletons.perl $5/e $5/f $5/align > $5/Singletons

echo 'Converting Bilingual Sentence Pair into Operation Corpus'

./generateSequences $5/e $5/f $5/align $5/Singletons > $5/opCorpus			# Generates Operation Corpus

echo 'Learning Operation Sequence Translation Model'

ngram-count -kndiscount -order $4 -unk -text $5/opCorpus -lm $5/operationLM$4

../../bin/build_binary -i $5/operationLM$4 $5/operationLM$4.bin

echo 'Training OSM - End'
date

