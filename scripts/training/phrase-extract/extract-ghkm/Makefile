.DELETE_ON_ERROR:
.PHONY: all clean

CC          = g++
CFLAGS      = -pedantic -Wall -O -g
CPPFLAGS    = -I ../

PROG        = extract-ghkm
SOURCES     = $(addsuffix .cpp,$(PROG)) \
              Alignment.cpp \
              AlignmentGraph.cpp \
              ParseTree.cpp \
              Span.cpp \
              Subgraph.cpp \
              ../SyntaxTree.cpp \
              ../XmlTree.cpp \
              XmlTreeParser.cpp
OBJS        = $(notdir $(SOURCES:.cpp=.o))

all: $(PROG)

clean:
	@rm -f $(PROG) $(OBJS)

$(PROG):  $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDFLAGS) -o $@

include Makefile.dep

Makefile.dep: $(SOURCES)
	$(CC) $(CPPFLAGS) -MM $(SOURCES) > $@

%.o:
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<
