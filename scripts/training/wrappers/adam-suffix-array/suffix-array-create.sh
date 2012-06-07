#!/bin/bash

# execute: ~/workspace/bin/moses-smt/scripts/training/wrappers/suffix-array-create.sh $SA_EXEC_DIR $SOURCE_CORPUS $TARGET_CORPUS $ALIGNMENT $SA_OUTPUT


SA_EXEC_DIR=$1
SOURCE_CORPUS=$2
TARGET_CORPUS=$3
ALIGNMENT=$4
SA_OUTPUT=$5
GLUE_GRAMMAR=$6

mkdir $SA_OUTPUT

rm -rf $SA_OUTPUT/bitext

pushd .
cd $SA_EXEC_DIR

./sa-compile.pl -output $SA_OUTPUT -b bitext_name=$SOURCE_CORPUS,$TARGET_CORPUS -a alignment_name=$ALIGNMENT > $SA_OUTPUT/extract.ini

popd

echo "<s> [X] ||| <s> [S] ||| 1 ||| ||| 0\n" > $GLUE_GRAMMAR
echo "[X][S] </s> [X] ||| [X][S] </s> [S] ||| 1 ||| 0-0 ||| 0\n" >> $GLUE_GRAMMAR
echo "[X][S] [X][X] [X] ||| [X][S] [X][X] [S] ||| 2.718 ||| 0-0 1-1 ||| 0\n" >> $GLUE_GRAMMAR
