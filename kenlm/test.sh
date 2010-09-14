#!/bin/bash
#Run tests.  Requires Boost.
set -e
./compile.sh
for i in util/{file_piece,joint_sort,key_value_packing,probing_hash_table,sorted_uniform}_test lm/ngram_test; do
  g++ -I. -O3 $i.cc {lm,util}/*.o -lboost_test_exec_monitor -o $i
  pushd $(dirname $i) && ./$(basename $i); popd
done 
