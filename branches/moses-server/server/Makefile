#
# Temporary makefile until I get xmlrpc-c working with automake
#

OBJS= 
XMLRPC_C_CONFIG=/home/bhaddow/xmlrpc-c/bin/xmlrpc-c-config
SRILM=/home/bhaddow/statmt/srilm

#ifndef DEBUG
#CFLAGS=-O3 -DTRACE_ENABLE -Wall
#else
#CFLAGS=-DTRACE_ENABLE -g -DDEBUG -Wall
#endif

CPPFLAGS=-I../moses/src $(shell $(XMLRPC_C_CONFIG) abyss-server --cflags) -g -Wall

GCC=g++
#LDFLAGS=$(shell $(XMLRPC_C_CONFIG) abyss-server --ldflags)
LDLIBS=-L../moses/src -L/home/bhaddow/protobuf/lib -L/home/bhaddow/statmt/srilm//lib/i686 -L/home/bhaddow/statmt/irstlm//lib/i686-redhat-linux-gnu -lmoses -lprotobuf -loolm -ldstruct -lmisc -lirstlm -Bstatic $(shell $(XMLRPC_C_CONFIG) c++2 abyss-server --libs) -Bdynamic  -lm -lz 

all: \
mosesserver

clean: 
	rm -f *.o mosesserver

%.o : %.cpp %.h
	$(GCC) -c $(CPPFLAGS) $< -o $@

mosesserver: $(OBJS) mosesserver.o
	$(GCC) $@.o $(OBJS) $(LDLIBS) -o $@ 
