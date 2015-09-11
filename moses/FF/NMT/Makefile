#!/bin/bash -ex
CC=g++
SOURCES=NMT_Wrapper.cpp test.cpp
STATEPATH=/home/tomaszd/work/nmt2moses/nmt_model/state.pkl
MODELPATH=/home/tomaszd/work/nmt2moses/nmt_model/min_en_de_model.npz
WRAPPERPATH=/home/tomaszd/work/nmt2moses/nmt/wrapper/python

test: $(SOURCES)
	$(CC) -I/usr/include/python2.7 -L/usr/lib/python2.7/config-x86_64-linux-gnu $^ -lpython2.7 -o $@ -ggdb

run:
	./test $(STATEPATH) $(MODELPATH) $(WRAPPERPATH)

.PHONY: run
