
# where to find include files and libraries from Moses
MOSES_INC=../../../moses/src ../../..
LIB_DIR=../../../moses/src/

LIBS=-lmoses -lz
OBJS=Main.o NBest.o Hypo.o Tools.o ParameterNBest.o

CFLAGS=-I$(MOSES_INC)

nbest-tool: $(OBJS)
	$(CXX) -o nbest $(OBJS) -L$(LIB_DIR) $(LIBS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

clean:
	-rm $(OBJS) nbest
