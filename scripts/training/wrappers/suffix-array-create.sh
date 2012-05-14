SA_EXEC_DIR=/Users/hieuhoang/workspace/github/cdec/sa-extract
WORKING_DIR=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/
EXP_NUM=2
SOURCE_LANG=fr
TARGET_LANG=en

SA_DIR=$WORKING_DIR/model/suffix-array.$EXP_NUM
mkdir $SA_DIR

rm -rf $SA_DIR/bitext

pushd .
cd $SA_EXEC_DIR

./sa-compile.pl -output $SA_DIR -b bitext_name=$WORKING_DIR/training/corpus.$EXP_NUM.$SOURCE_LANG,$WORKING_DIR/training/corpus.$EXP_NUM.$TARGET_LANG -a alignment_name=$WORKING_DIR/model/aligned.$EXP_NUM.grow-diag-final-and > $SA_DIR/extract.ini

popd

