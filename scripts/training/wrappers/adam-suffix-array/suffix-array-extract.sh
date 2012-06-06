#!/bin/bash

# execute: ~/workspace/bin/moses-smt/scripts/training/wrappers/adam-suffix-array/suffix-array-extract.sh $SA_EXEC_DIR $MODEL_DIR $INPUT_FILE $OUTPUT_DIR

SA_EXEC_DIR=$1
MODEL_DIR=$2
INPUT_FILE=$3
OUTPUT_DIR=$4

mkdir $OUTPUT_DIR

pushd .
cd $OUTPUT_DIR

cat $INPUT_FILE | $SA_EXEC_DIR/escape-testset.pl | $SA_EXEC_DIR/extractor.py -c  $MODEL_DIR/extract.ini

popd

