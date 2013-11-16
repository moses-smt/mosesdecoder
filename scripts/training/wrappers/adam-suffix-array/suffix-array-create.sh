#!/bin/bash

# execute: ~/workspace/bin/moses-smt/scripts/training/wrappers/suffix-array-create.sh $SA_EXEC_DIR $SOURCE_CORPUS $TARGET_CORPUS $ALIGNMENT $SA_OUTPUT

# eg.
#SA_EXEC_DIR=/Users/hieuhoang/workspace/github/cdec/sa-extract
#SOURCE_CORPUS=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/training/corpus.2.fr
#TARGET_CORPUS=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/training/corpus.2.en
#ALIGNMENT=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/model/aligned.3.grow-diag-final-and
#SA_OUTPUT=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/model/suffix-array.3


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

python $SA_EXEC_DIR/cdec/sa/compile.py -o $SA_OUTPUT -f $SOURCE_CORPUS -e $TARGET_CORPUS -a $ALIGNMENT -c $SA_OUTPUT/extract.ini

popd

echo "<s> [X] ||| <s> [S] ||| 1 ||| ||| 0" > $GLUE_GRAMMAR
echo "[X][S] </s> [X] ||| [X][S] </s> [S] ||| 1 ||| 0-0 ||| 0" >> $GLUE_GRAMMAR
echo "[X][S] [X][X] [X] ||| [X][S] [X][X] [S] ||| 2.718 ||| 0-0 1-1 ||| 0" >> $GLUE_GRAMMAR
