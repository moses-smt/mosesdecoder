#!/bin/bash
g++ -O3 -I. -licui18n lm/arpa_io.cc lm/exception.cc lm/ngram.cc lm/query.cc lm/virtual_interface.cc util/errno_exception.cc util/file_piece.cc util/murmur_hash.cc util/scoped.cc util/string_piece.cc -o query -licutu -licutu -licudata -licuio -licule -liculx -licuuc
