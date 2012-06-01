SA_EXEC_DIR=/Users/hieuhoang/workspace/github/cdec/sa-extract
MODEL_DIR=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/model/suffix-array.3

INPUT_FILE=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/tuning/input.lc.2
OUTPUT_DIR=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/tuning/filtered.sa.3

~/workspace/bin/moses-smt/scripts/training/wrappers/adam-suffix-array/suffix-array-extract.sh $SA_EXEC_DIR $MODEL_DIR $INPUT_FILE $OUTPUT_DIR

