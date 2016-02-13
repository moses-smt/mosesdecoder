#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <boost/timer/timer.hpp>

#include "mblas/matrix.h"
#include "model.h"
#include "encoder.h"
#include "decoder.h"
#include "vocab.h"

#include "states.h"

using namespace mblas;

int main(int argc, char** argv) {
  size_t device = 0;
  
  if(argc > 1)
    device = 1;
  
  cudaSetDevice(device);
  
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
  size_t bs = 500;
  std::vector<std::vector<size_t>> tWordsBatch = {
    Batch(bs, tvcb["das"]),
    Batch(bs, tvcb["ist"]),
    Batch(bs, tvcb["ein"]),
    Batch(bs, tvcb["kleiner"]),
    Batch(bs, tvcb["test"]),
    Batch(bs, tvcb["."]),
    Batch(bs, tvcb["</s>"])
  };
  
  std::vector<size_t> filter = {
    tvcb["das"], tvcb["ist"], tvcb["ein"], tvcb["kleiner"], tvcb["test"],
    tvcb["."], tvcb["</s>"], 0, tvcb["dies"], tvcb["ist"], tvcb["war"],        
    tvcb["eine"], tvcb["ganz"], tvcb["kleine"], tvcb["frau"], 
  };
    
  decoder.Filter(filter); // Limit to allowed vocabulary
    
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
  
  for(size_t i = 0; i < 10; ++i) {
    decoder.EmptyState(PrevState, SourceContext, batchSize);
    decoder.EmptyEmbedding(PrevEmbedding, batchSize);
    
    size_t k = 0;
    for(auto w : tWordsBatch) {
    
      decoder.GetProbs(Probs, AlignedSourceContext,
                       PrevState, PrevEmbedding, SourceContext);
      
      
      float p = Probs(0, k);
      std::cerr << k << " " << filter[k++] << " " << p << std::endl;
      
      decoder.Lookup(Embedding, w);
      decoder.GetNextState(State, Embedding,
                           PrevState, AlignedSourceContext);
      
      mblas::Swap(State, PrevState);
      mblas::Swap(Embedding, PrevEmbedding);
    }
  }
}
