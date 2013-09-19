#!/bin/bash
cd "$(dirname "$0")/.."
rm -rf {lm,util}/*.o lm/query lm/build_binary {lm,util}/*_test lm/test.binary* lm/test.arpa?????? util/file_piece.cc.gz
