#!/bin/bash

P=$1

# source language (example: fr)
S=$2
# target language (example: en)
T=$3

# path to nematus/data
P1=$4

# path to subword NMT scripts (can be downloaded from https://github.com/rsennrich/subword-nmt)
P2=$5

# tokenize
perl $P1/tokenizer.perl -threads 5 -l $S < {P}.${S} > {P}.${S}.tok
perl $P1/tokenizer.perl -threads 5 -l $T < {P}.${T} > {P}.${T}.tok

# learn BPE on joint vocabulary:
cat {P}.${S}.tok {P}.${T}.tok | python $P2/learn_bpe.py -s 20000 > ${S}${T}.bpe

python $P2/apply_bpe.py -c ${S}${T}.bpe < {P}.${S}.tok > {P}.${S}.tok.bpe
python $P2/apply_bpe.py -c ${S}${T}.bpe < {P}.${T}.tok > {P}.${T}.tok.bpe

# build dictionary
python $P1/build_dictionary.py {P}.${S}.tok.bpe
python $P1/build_dictionary.py {P}.${T}.tok.bpe

