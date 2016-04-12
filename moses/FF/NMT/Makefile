all: test_cuda rescorer_cuda test_dl4mt

NMTCU = plugin/nmt.cu cnpy/cnpy.cpp
NMT = plugin/nmt.cpp cnpy/cnpy.cpp
SOURCESCU = test/test.cu cnpy/cnpy.cpp
DL4MTCU = test/test.dl4mt.cu cnpy/cnpy.cpp
RESCORERCU = rescorer/rescorer_main.cu common/utils.cu rescorer/nbest.cu cnpy/cnpy.cpp
RESCORER_HEADERS = rescorer/nbest.h
SOURCES = test/test.cpp cnpy/cnpy.cpp
HEADERS = bahdanau.h common/states.h common/npz_converter.h common/vocab.h mblas/matrix.h mblas/base_matrix.h
DL2MTHEADERS = dl4mt.h common/states.h common/npz_converter.h common/vocab.h mblas/matrix.h mblas/base_matrix.h

NDEC_HEADERS = decoder/hypothesis_manager.h decoder/hypothesis.h decoder/nmt_decoder.h decoder/result.h
NDEC_SOURCES = decoder/decoder_main.cu common/utils.cu cnpy/cnpy.cpp

test_cuda: $(SOURCESCU) $(HEADERS)
	nvcc --use_fast_math -O3 -Icommon -I. -std=c++11 $(SOURCESCU) -arch=sm_35 -lineinfo -lboost_system -lboost_timer -lcudart -lcublas -o $@

ndec: $(NDEC_HEADERS) $(NDEC_SOURCES)
	nvcc --use_fast_math -O3 -Icommon -I. -std=c++11 $(NDEC_SOURCES) -arch=sm_35 -lboost_program_options -lineinfo -lboost_system -lboost_timer -lcudart -lcublas -o $@

test_dl4mt: $(DL4MTCU) $(DL4MTHEADERS)
	nvcc --use_fast_math -O3 -Icommon -I. -std=c++11 $(DL4MTCU) -arch=sm_35 -lineinfo -lboost_system -lboost_timer -lcudart -lcublas -o $@

%.o : %.cu
	nvcc -O3 -std=c++11 -c $^ -o $@

rescorer_cuda: $(RESCORERCU) $(HEADERS) $(RESCORER_HEADERS)
	nvcc --use_fast_math -O3 -std=c++11 $(RESCORERCU) -Icommon -I. -lboost_system -lboost_timer -lboost_program_options -lcudart -lcublas -o $@

clean:
	rm -rf test_cuda test_gsl test_atlas test_ptatlas libnmt-cuda.so rescorer *.o *.so

