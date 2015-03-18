#!/bin/bash

if [ $# -eq 2 ]; then
    OUTFILE=$1
    WORKDIR=$2
else
    echo "usage: $0 <outfile> <working_directory>"
    exit 1
fi

NPLM=/path/to/nplm
MOSES_ROOT=/path/to/mosesdecoder

INFILE=/path/to/file/in/moses/xml/format
VALIDATIONFILE=/path/to/file/in/moses/xml/format
#TESTFILE1=/path/to/file/in/moses/xml/format
#TESTFILE2=/path/to/file/in/moses/xml/format
PREFIX=$(basename $OUTFILE)

EPOCHS=1
INPUT_VOCAB_SIZE=500000
OUTPUT_VOCAB_SIZE=75
MINIBATCH_SIZE=1000
NOISE=50
HIDDEN=0
INPUT_EMBEDDING=150
OUTPUT_EMBEDDING=750
THREADS=4
MODE=label
UP_CONTEXT=2
LEFT_CONTEXT=3
RIGHT_CONTEXT=0


mkdir -p $WORKDIR

python $MOSES_ROOT/scripts/training/rdlm/extract_vocab.py --output $WORKDIR/vocab < $INFILE || exit 1

head -n $INPUT_VOCAB_SIZE $WORKDIR/vocab.all > $WORKDIR/vocab.input
cat $WORKDIR/vocab_target.special $WORKDIR/vocab_target.nonterminals |
    grep -v "^<null" |
    grep -v "^<root" |
    grep -v "^<start_head" |
    grep -v "^<dummy" |
    grep -v "^<head_head" |
    grep -v "^<stop_head" |
    head -n $OUTPUT_VOCAB_SIZE > $WORKDIR/vocab.output

python $MOSES_ROOT/scripts/training/rdlm/extract_syntactic_ngrams.py --vocab $WORKDIR/vocab.input --output_vocab $WORKDIR/vocab.output \
    --mode $MODE --left_context $LEFT_CONTEXT --right_context $RIGHT_CONTEXT --up_context $UP_CONTEXT < $INFILE > $WORKDIR/train.ngrams || exit 1
python $MOSES_ROOT/scripts/training/rdlm/extract_syntactic_ngrams.py --vocab $WORKDIR/vocab.input --output_vocab $WORKDIR/vocab.output \
    --mode $MODE --left_context $LEFT_CONTEXT --right_context $RIGHT_CONTEXT --up_context $UP_CONTEXT < $VALIDATIONFILE > $WORKDIR/validation.ngrams || exit 1

$NPLM/src/trainNeuralNetwork --train_file $WORKDIR/train.ngrams --validation_file $WORKDIR/validation.ngrams \
   --num_epochs $EPOCHS --input_words_file $WORKDIR/vocab.input --output_words_file $WORKDIR/vocab.output --model_prefix $WORKDIR/$PREFIX \
   --input_vocab_size $INPUT_VOCAB_SIZE --output_vocab_size $OUTPUT_VOCAB_SIZE \
   --learning_rate 1 --minibatch_size $MINIBATCH_SIZE --num_noise_samples $NOISE --num_hidden $HIDDEN \
   --input_embedding_dimension $INPUT_EMBEDDING --output_embedding_dimension $OUTPUT_EMBEDDING --num_threads $THREADS || exit 1

python $MOSES_ROOT/scripts/training/rdlm/average_null_embedding.py $NPLM $WORKDIR/$PREFIX.$(($EPOCHS)) $WORKDIR/train.ngrams $OUTFILE || exit 1

if [[ $TESTFILE1 ]]; then
  python $MOSES_ROOT/scripts/training/rdlm/extract_syntactic_ngrams.py --vocab $WORKDIR/vocab.input --output_vocab $WORKDIR/vocab.output \
    --mode $MODE --left_context $LEFT_CONTEXT --right_context $RIGHT_CONTEXT --up_context $UP_CONTEXT < $TESTFILE1 > $WORKDIR/test1.ngrams || exit 1
  $NPLM/src/testNeuralNetwork --test_file $WORKDIR/test1.ngrams --model_file $OUTFILE --minibatch_size $MINIBATCH_SIZE --num_threads $THREADS || exit 1
fi

if [[ $TESTFILE2 ]]; then
  python $MOSES_ROOT/scripts/training/rdlm/extract_syntactic_ngrams.py --vocab $WORKDIR/vocab.input --output_vocab $WORKDIR/vocab.output \
    --mode $MODE --left_context $LEFT_CONTEXT --right_context $RIGHT_CONTEXT --up_context $UP_CONTEXT < $TESTFILE2 > $WORKDIR/test2.ngrams || exit 1
  $NPLM/src/testNeuralNetwork --test_file $WORKDIR/test2.ngrams --model_file $OUTFILE --minibatch_size $MINIBATCH_SIZE --num_threads $THREADS || exit 1
fi