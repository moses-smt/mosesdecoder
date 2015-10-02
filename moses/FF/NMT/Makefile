#!/bin/bash -ex
CC=g++
SOURCES=../../../util/exception.o NMT_Wrapper.o test.o
STATEPATH=/work/nmt_model/state.pkl
MODELPATH=/work/nmt_model/min_en_de_model.npz
SOURCEVOCAB=/work/nmt_model/vocab/mini_en_de_.en.pkl
TARGETVOCAB=/work/nmt_model/vocab/mini_en_de_.de.pkl
WRAPPERPATH=./wrapper


test: $(SOURCES)
	$(CC) -L/usr/lib/python2.7/config-x86_64-linux-gnu $^ -lpython2.7  -o $@ 

../../../util/exception.o:../../../util/exception.cc
	$(CC) -I/work/mosesdecoder -fPIC -c $< -o $@

test.o: test.cpp
	$(CC) -O3 -Wall $< -c -o $@

%.o: %.cpp
	$(CC) -fPIC -ftemplate-depth-128 -O3 -finline-functions -Wno-inline -Wall -pthread -I/work/mosesdecoder -I/usr/include/python2.7 -L/usr/lib/python2.7/config-x86_64-linux-gnu $^ -lpython2.7 -c -o $@

run:
	./test $(STATEPATH) $(MODELPATH) $(WRAPPERPATH) $(SOURCEVOCAB) $(TARGETVOCAB)

run2:
	LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:`pwd` ./test2 $(STATEPATH) $(MODELPATH) $(WRAPPERPATH) $(SOURCEVOCAB) $(TARGETVOCAB)

.PHONY: run run2

test2: test.o libwrapper.so
	$(CC) -L/usr/lib/python2.7/config-x86_64-linux-gnu -L. $< -lwrapper -lpython2.7 -o $@

libwrapper.so: NMT_Wrapper.o ../../../util/exception.o
	g++ -L/usr/lib/python2.7/config-x86_64-linux-gnu -fPIC -shared -Wall -o $@ -lpython2.7 $^
