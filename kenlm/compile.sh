#!/bin/bash
#This is just an example compilation.  You should integrate these files into your build system.  I can provide boost jam if you want.  
#If your code uses ICU, edit util/string_piece.hh and uncomment #define USE_ICU

set -e

for i in util/{ersatz_progress,exception,file_piece,murmur_hash,scoped,string_piece} lm/{exception,virtual_interface,ngram}; do
  g++ -I. -O3 -c $i.cc -o $i.o
done
g++ -I. -O3 lm/ngram_build_binary.cc {lm,util}/*.o -o build_binary
g++ -I. -O3 lm/ngram_query.cc {lm,util}/*.o -o query
