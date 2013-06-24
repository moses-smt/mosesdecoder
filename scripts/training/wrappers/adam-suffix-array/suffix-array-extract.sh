#!/bin/bash

# execute: ~/workspace/bin/moses-smt/scripts/training/wrappers/adam-suffix-array/suffix-array-extract.sh $SA_EXEC_DIR $MODEL_DIR $INPUT_FILE $OUTPUT_DIR

# eg.
#SA_EXEC_DIR=/Users/hieuhoang/workspace/github/cdec/sa-extract
#MODEL_DIR=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/model/suffix-array.3
#INPUT_FILE=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/tuning/input.lc.2
#OUTPUT_DIR=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/tuning/filtered.sa.3

SA_EXEC_DIR=$1
MODEL_DIR=$2
INPUT_FILE=$3
OUTPUT_DIR=$4
JOBS=$5

mkdir $OUTPUT_DIR

pushd .
cd $OUTPUT_DIR

cat $INPUT_FILE | python $SA_EXEC_DIR/cdec/sa/extract.py -c $MODEL_DIR/extract.ini -g $OUTPUT_DIR -j $JOBS -z > $OUTPUT_DIR/input.sgm

popd

