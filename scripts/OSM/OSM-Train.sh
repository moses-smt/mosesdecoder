#!/bin/sh

echo 'Training OSM - Start'
date

mkdir $5
ln -s $1 $5/e
ln -s $2 $5/f

$6/scripts/OSM/flipAlignment $3 > $5/align

echo 'Extracting Singletons'

$6/scripts/OSM/extract-singletons.perl $5/e $5/f $5/align > $5/Singletons

echo 'Converting Bilingual Sentence Pair into Operation Corpus'

$6/scripts/OSM/generateSequences $5/e $5/f $5/align $5/Singletons > $5/opCorpus			# Generates Operation Corpus

echo 'Learning Operation Sequence Translation Model'

$7/ngram-count -kndiscount -order $4 -unk -text $5/opCorpus -lm $5/operationLM

echo 'Binarizing'

$6/bin/build_binary $5/operationLM $5/operationLM.bin

\rm $5/e
\rm $5/f
\rm $5/align

echo 'Training OSM - End'
date

