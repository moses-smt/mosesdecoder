#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/timer/timer.hpp>
#include <boost/algorithm/string.hpp>

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
  
  std::string source = "you know , one of the intense pleasures of travel and one of the delights of ethnographic research is the opportunity to live amongst those who have not forgotten the old ways , who still feel their past in the wind , touch it in stones polished by rain , taste it in the bitter leaves of plants .";
  
  std::string target = "wissen sie , ein intensives vergnügen reisen und die freuden der ethnographischen forschung ist die möglichkeit , unter denen leben nicht vergessen , die alte art , die sich ihrer vergangenheit noch im wind , berühren sie in steine poliert von regen , geschmack in den bitteren blätter von pflanzen .";
 
  std::cerr << "Loading model" << std::endl;
  Weights weights("/home/marcinj/Badania/best_nmt/search_model.npz", device);
  Vocab svcb("/home/marcinj/Badania/best_nmt/vocab/en_de.en.txt");
  Vocab tvcb("/home/marcinj/Badania/best_nmt/vocab/en_de.de.txt");
  
  std::cerr << "Creating encoder" << std::endl;
  Encoder encoder(weights);
  std::cerr << "Creating decoder" << std::endl;
  Decoder decoder(weights);
  
  std::vector<std::string> sourceSplit;
  boost::split(sourceSplit, source, boost::is_any_of(" "),
               boost::token_compress_on);
    
  std::cerr << "Source: " << std::endl;
  std::vector<size_t> sWords(sourceSplit.size());
  std::transform(sourceSplit.begin(), sourceSplit.end(), sWords.begin(),
                 [&](const std::string& w) { std::cerr << svcb[w] << ", "; return svcb[w]; });
  sWords.push_back(svcb["</s>"]);
  std::cerr << svcb["</s>"] << std::endl;
  
  
  typedef std::vector<size_t> Batch;
  
  std::vector<std::string> targetSplit;
  boost::split(targetSplit, target, boost::is_any_of(" "),
               boost::token_compress_on);
    
  std::cerr << "Target: " << std::endl;
  size_t bs = 100;
  std::vector<std::vector<size_t>> tWordsBatch(targetSplit.size());
  std::transform(targetSplit.begin(), targetSplit.end(), tWordsBatch.begin(),
                 [&](const std::string& w) { std::cerr << tvcb[w] << ", "; return Batch(bs, tvcb[w]); });
  tWordsBatch.push_back(Batch(bs, tvcb["</s>"]));
  std::cerr << tvcb["</s>"] << std::endl;
  
  
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
  
  for(size_t i = 0; i < 1; ++i) {
    decoder.EmptyState(PrevState, SourceContext, batchSize);
    decoder.EmptyEmbedding(PrevEmbedding, batchSize);
    
    float sum = 0;
    for(auto w : tWordsBatch) {
    
      decoder.GetProbs(Probs, AlignedSourceContext,
                       PrevState, PrevEmbedding, SourceContext);
      
      
      float p = Probs(0, w[0]);
      sum += log(p);
      
      decoder.Lookup(Embedding, w);
      decoder.GetNextState(State, Embedding,
                           PrevState, AlignedSourceContext);
      
      mblas::Swap(State, PrevState);
      mblas::Swap(Embedding, PrevEmbedding);
    }
    
    std::cerr << sum << std::endl;
    
  }
}
