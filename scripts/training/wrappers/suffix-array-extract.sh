INPUT_FILE=/Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/training/10
OUTPUT_PATH=/Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/sa/filtered

SA_EXEC_DIR=/Users/hieuhoang/workspace/github/cdec/sa-extract
WORKING_DIR=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/
EXP_NUM=2

SA_DIR=$WORKING_DIR/model/suffix-array.$EXP_NUM

mkdir $OUTPUT_PATH

pushd .
cd $OUTPUT_PATH

$SA_EXEC_DIR/extractor.py -c  $SA_DIR/extract.ini < $INPUT_FILE

popd

