#!/bin/sh

WRAP_DIR=~/moses.new/scripts/training/wrappers/


tagger=$WRAP_DIR/make-factor-en-pos.mxpost.perl
lang=en
for stem  in test train.10k train.100k; do
  $tagger -mxpost /home/pkoehn/statmt/project/mxpost $stem.$lang $stem.tagged.$lang /tmp
done

tagger=$WRAP_DIR/make-factor-de-pos.perl
lang=de
for stem  in test train.10k train.100k; do
  $tagger $stem.$lang $stem.tagged.$lang /tmp
done

