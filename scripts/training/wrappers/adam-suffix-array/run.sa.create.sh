SA_EXEC_DIR=/Users/hieuhoang/workspace/github/cdec/sa-extract
SOURCE_CORPUS=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/training/corpus.2.fr
TARGET_CORPUS=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/training/corpus.2.en
ALIGNMENT=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/model/aligned.3.grow-diag-final-and
SA_OUTPUT=/Users/hieuhoang/workspace/data/europarl/exp/fr-en/model/suffix-array.3

~/workspace/bin/moses-smt/scripts/training/wrappers/suffix-array-create.sh $SA_EXEC_DIR $SOURCE_CORPUS $TARGET_CORPUS $ALIGNMENT $SA_OUTPUT

