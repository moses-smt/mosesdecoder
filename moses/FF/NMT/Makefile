all: test_cuda libnmt-cuda.so

NMTCU = nmt.cu cnpy/cnpy.cpp 
SOURCESCU = test.cu cnpy/cnpy.cpp 
SOURCES = test.cpp cnpy/cnpy.cpp 
HEADERS = decoder.h encoder.h model.h npz_converter.h vocab.h mblas/matrix.h mblas/base_matrix.h

test_cuda: $(SOURCESCU) $(HEADERS)
	nvcc -O3 -std=c++11 $(SOURCESCU) -lboost_system -lboost_timer -lcudart -lcublas -o $@

#test.cpp: test.cu
#	cp $^ $@

#test_gsl: $(SOURCES) $(HEADERS)
#	g++-5 -O3 -Ofast -std=c++11 $(SOURCES) -DNO_CUDA \
#	-lgslcblas -ftree-vectorize -msse2 -lboost_system -lboost_timer -o $@

#test_atlas: $(SOURCES) $(HEADERS)
#	g++-5 -O3 -std=c++11 $(SOURCES) -DNO_CUDA \
#	-msse2 -Ofast \
#	-I/home/marcinj/Badania/nmtgit/ATLAS/include -L/home/marcinj/Badania/nmtgit/ATLAS/atlasbuild/lib -lcblas -latlas -pthread \
#	-march=native -lboost_system -lboost_timer -o $@ \

#test_ptatlas: $(SOURCES) $(HEADERS)
#	g++-5 -O3 -Ofast -std=c++11 $(SOURCES) -DNO_CUDA \
#	-I/home/marcinj/Badania/nmtgit/ATLAS/include -L/home/marcinj/Badania/nmtgit/ATLAS/atlasbuild2/lib \
#	-lptcblas -latlas -pthread \
#	-march=native -lboost_system -lboost_timer -o $@ \

libnmt-cuda.so: $(NMTCU) $(HEADERS)
	nvcc -shared -O3 -std=c++11 $(NMTCU) -lboost_system -lboost_timer -lcudart -lcublas --compiler-options '-fPIC'  -o $@


clean:
	rm -rf test_cuda test_gsl test_atlas test_ptatlas libnmt-cuda.so

