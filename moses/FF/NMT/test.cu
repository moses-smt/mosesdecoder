#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <boost/timer/timer.hpp>

#include "mblas/matrix.h"
#include "model.h"
#include "encoder.h"
#include "decoder.h"
#include "vocab.h"

using namespace mblas;

int main(int argc, char** argv) {
  size_t device = 0;
  
  if(argc > 1)
    device = 1;
  
  cudaSetDevice(device);
  //CublasHandle::Init(device);
  
  std::cerr << "Loading model" << std::endl;
  Weights weights("/home/marcinj/Badania/nmt/en_de_1/search_model.npz", device);
  Vocab svcb("/home/marcinj/Badania/nmt/en_de_1/src.vocab.txt");
  Vocab tvcb("/home/marcinj/Badania/nmt/en_de_1/trg.vocab.txt");
  
  std::cerr << "Creating encoder" << std::endl;
  Encoder encoder(weights);
  std::cerr << "Creating decoder" << std::endl;
  Decoder decoder(weights);
  
  std::vector<size_t> sWords = {svcb["this"], svcb["is"], svcb["a"],
                                svcb["little"], svcb["test"], svcb["."],
                                svcb["</s>"]};
  
  //std::vector<std::vector<size_t>> tWordsBatch = {
  //  {  tvcb["das"],     tvcb["dies"],    tvcb["das"]     },
  //  {  tvcb["ist"],     tvcb["war"],     tvcb["ist"]     },
  //  {  tvcb["ein"],     tvcb["ein"],     tvcb["eine"]    },
  //  {  tvcb["kleiner"], tvcb["ganz"],    tvcb["kleine"]  },
  //  {  tvcb["test"],    tvcb["kleiner"], tvcb["frau"]    },
  //  {  tvcb["."],       tvcb["test"],    tvcb["."]       },
  //  {  tvcb["</s>"],    tvcb["."],       tvcb["</s>"]    },
  //  {  0,               tvcb["</s>"],    0               }
  //};
  
  typedef std::vector<size_t> Batch;
  size_t bs = 1;
  std::vector<std::vector<size_t>> tWordsBatch = {
    Batch(bs, tvcb["das"]),
    Batch(bs, tvcb["ist"]),
    Batch(bs, tvcb["ein"]),
    Batch(bs, tvcb["kleiner"]),
    Batch(bs, tvcb["test"]),
    Batch(bs, tvcb["."]),
    Batch(bs, tvcb["</s>"])
  };
    
  mblas::Matrix SourceContext;
  encoder.GetContext(sWords, SourceContext);
  
  mblas::Matrix PrevState;
  mblas::Matrix PrevEmbedding;

  mblas::Matrix AlignedSourceContext;
  mblas::Matrix Probs;
  
  mblas::Matrix State;
  mblas::Matrix Embedding;

  std::cerr << "Testing" << std::endl;
  boost::timer::auto_cpu_timer timer;
  size_t batchSize = tWordsBatch[0].size();

  decoder.EmptyState(PrevState, SourceContext, batchSize);
  decoder.EmptyEmbedding(PrevEmbedding, batchSize);
  
  float sum = 0;
  for(auto w : tWordsBatch) {
  
    decoder.GetProbs(Probs, AlignedSourceContext,
                     PrevState, PrevEmbedding, SourceContext);
    
    for(size_t j = 0; j < 1; ++j) {
      float p = Probs(j, w[j]);
      std::cerr << j << " " << w[j] << " " << log(p) << std::endl;
      sum += log(p);
    }
  
    decoder.Lookup(Embedding, w);
    decoder.GetNextState(State, Embedding,
                         PrevState, AlignedSourceContext);
    
    mblas::Swap(State, PrevState);
    mblas::Swap(Embedding, PrevEmbedding);
  }
}
