#!/bin/bash

# execute: ~/workspace/bin/moses-smt/scripts/training/wrappers/suffix-array-extract.sh $SA_EXEC_DIR $MODEL_DIR $INPUT_FILE $OUTPUT_DIR

SA_EXEC_DIR=$1
MODEL_DIR=$2
INPUT_FILE=$3
OUTPUT_DIR=$4

mkdir $OUTPUT_DIR

pushd .
cd $OUTPUT_DIR

$SA_EXEC_DIR/extractor.py -c  $MODEL_DIR/extract.ini < $INPUT_FILE

popd

