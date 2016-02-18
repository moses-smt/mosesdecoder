all: test_cuda rescorer_cuda

NMTCU = plugin/nmt.cu cnpy/cnpy.cpp
NMT = plugin/nmt.cpp cnpy/cnpy.cpp
SOURCESCU = test/test.cu cnpy/cnpy.cpp
RESCORERCU = rescorer/rescorer_main.cu common/utils.cu rescorer/nbest.cu cnpy/cnpy.cpp
SOURCES = test/test.cpp cnpy/cnpy.cpp
HEADERS = common/decoder.h common/encoder.h common/model.h common/states.h common/npz_converter.h common/vocab.h mblas/matrix.h mblas/base_matrix.h

test_cuda: $(SOURCESCU) $(HEADERS)
	nvcc --use_fast_math -O3 -Icommon -I. -std=c++11 $(SOURCESCU) -arch=sm_35 -lineinfo -lboost_system -lboost_timer -lcudart -lcublas -o $@

%.o : %.cu
	nvcc -O3 -std=c++11 -c $^ -o $@

rescorer_cuda: $(RESCORERCU) $(HEADERS)
	nvcc --use_fast_math -O3 -std=c++11 $(RESCORERCU) -Icommon -I. -lboost_system -lboost_timer -lboost_program_options -lcudart -lcublas -o $@

clean:
	rm -rf test_cuda test_gsl test_atlas test_ptatlas libnmt-cuda.so rescorer *.o *.so

