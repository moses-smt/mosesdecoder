all: test_cuda rescorer

NMTCU = plugin/nmt.cu cnpy/cnpy.cpp
NMT = plugin/nmt.cpp cnpy/cnpy.cpp
SOURCESCU = test/test.cu cnpy/cnpy.cpp
RESCORERCU = rescorer/rescorer_main.cu common/utils.cu rescorer/nbest.cu cnpy/cnpy.cpp
SOURCES = test/test.cpp cnpy/cnpy.cpp
HEADERS = common/decoder.h common/encoder.h common/model.h common/states.h common/npz_converter.h common/vocab.h mblas/matrix.h mblas/base_matrix.h

test/test_cuda: $(SOURCESCU) $(HEADERS)
	nvcc -O3 -std=c++11 $(SOURCESCU) -Icommon -I. -lboost_system -lboost_timer -lcudart -lcublas -o $@

%.o : %.cu
	nvcc -O3 -std=c++11 -c $^ -o $@

test/test.cpp: test/test.cu
	cp $^ $@

nmt.cpp: nmt.cu
	cp $^ $@

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

plugin/libnmt-cuda.so: $(NMTCU) $(HEADERS)
	nvcc -shared -O3 -Icommon -I. -std=c++11 $(NMTCU) \
	-lboost_system -lboost_timer -lcudart -lcublas \
	--compiler-options '-fPIC'  -o $@

#libnmt-gsl.so: $(NMT) $(HEADERS)
#	g++-5 -shared -fPIC -O3 -Ofast -std=c++11 $(NMT) -DNO_CUDA \
#	-lgslcblas -ftree-vectorize -msse2 -lboost_system -lboost_timer -o $@
#
#
rescorer/rescorer: $(RESCORERCU) $(HEADERS)
	nvcc -O3 -std=c++11 $(RESCORERCU) -Icommon -I. -lboost_system -lboost_timer -lboost_program_options -lcudart -lcublas -o $@

clean:
	rm -rf test_cuda test_gsl test_atlas test_ptatlas libnmt-cuda.so rescorer *.o *.so

