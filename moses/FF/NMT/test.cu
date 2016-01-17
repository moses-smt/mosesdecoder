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

int main() {  
  std::cerr << "Loading model" << std::endl;
  Weights weights("../../nmt/en_de_1/search_model.npz");
  Vocab svcb("src.vocab.txt");
  Vocab tvcb("trg.vocab.txt");
  
  std::cerr << "Creating encoder" << std::endl;
  Encoder encoder(weights);
  std::cerr << "Creating decoder" << std::endl;
  Decoder decoder(weights);
  
  std::vector<size_t> sWords = {svcb["this"], svcb["is"], svcb["a"],
                                svcb["little"], svcb["test"], svcb["."],
                                svcb["</s>"]};
  
  std::vector<std::vector<size_t>> tWordsBatch = {
    {  tvcb["das"],     tvcb["dies"],    tvcb["das"]     },
    {  tvcb["ist"],     tvcb["war"],     tvcb["ist"]     },
    {  tvcb["ein"],     tvcb["ein"],     tvcb["eine"]    },
    {  tvcb["kleiner"], tvcb["ganz"],    tvcb["kleine"]  },
    {  tvcb["test"],    tvcb["kleiner"], tvcb["frau"]    },
    {  tvcb["."],       tvcb["test"],    tvcb["."]       },
    {  tvcb["</s>"],    tvcb["."],       tvcb["</s>"]    },
    {  0,               tvcb["</s>"],    0               }
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
    
    for(size_t j = 0; j < 3; ++j) {
      float p = Probs(j, w[j]);
      std::cerr << j << " " << w[j] << " " << p << std::endl;
      sum += log(p);
    }
  
    decoder.GetNextState(State, Embedding,
                         w, PrevState, AlignedSourceContext);
    //std::cerr << "State" << std::endl;
    //debug1(State);
    
    mblas::Swap(State, PrevState);
    mblas::Swap(Embedding, PrevEmbedding);
  }
  //std::cerr << sum << std::endl;    
}
