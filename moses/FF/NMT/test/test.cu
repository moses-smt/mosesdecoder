#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/timer/timer.hpp>
#include <boost/algorithm/string.hpp>

#include "mblas/matrix.h"
#include "bahdanau.h"
#include "vocab.h"

#include "states.h"

using namespace mblas;

int main(int argc, char** argv) {
  size_t device = 0;
  
  if(argc > 1) {
    if(std::string(argv[1]) == "1")
      device = 1;
    else if(std::string(argv[1]) == "2")
      device = 2;
  }
  
  std::cerr << device << std::endl;
  cudaSetDevice(device);
  
  std::string source = "thank you .";
  std::string target = "vielen dank .";
  //std::string source = "you know , one of the intense pleasures of travel and one of the delights of ethnographic research is the opportunity to live amongst those who have not forgotten the old ways , who still feel their past in the wind , touch it in stones polished by rain , taste it in the bitter leaves of plants .";
  //std::string target = "wissen sie , eine der intensiven freuden des reisens und eine der freuden der ethnografischen forschung ist die chance zu leben unter jenen , die die alten wege nicht vergessen haben , die immer noch ihre vergangenheit im wind spüren , berühren sie in steine poliert durch regen , schmecken sie in den bitteren blätter der pflanzen .";
  
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
  size_t bs = 1000;
  std::vector<std::vector<size_t>> tWordsBatch(targetSplit.size());
  std::transform(targetSplit.begin(), targetSplit.end(), tWordsBatch.begin(),
                 [&](const std::string& w) { std::cerr << tvcb[w] << ", "; return Batch(bs, tvcb[w]); });
  tWordsBatch.push_back(Batch(bs, tvcb["</s>"]));
  std::cerr << tvcb["</s>"] << std::endl;

  mblas::Matrix SourceContext;
  encoder.GetContext(sWords, SourceContext);

  mblas::Matrix State, NextState;
  mblas::Matrix Embeddings, NextEmbeddings;
  mblas::Matrix Probs;

  std::cerr << "Testing" << std::endl;
  boost::timer::auto_cpu_timer timer;
  size_t batchSize = tWordsBatch[0].size();
  
  for(size_t i = 0; i < 1; ++i) {
    decoder.EmptyState(State, SourceContext, batchSize);
    decoder.EmptyEmbedding(Embeddings, batchSize);
    
    float sum = 0;
    for(auto batch : tWordsBatch) {
      decoder.MakeStep(NextState, NextEmbeddings, Probs,
                       batch, State, Embeddings, SourceContext);

      for(size_t i = 0; i < 1; ++i) {
        float p = Probs(i, batch[i]);
        std:: cerr << log(p) << " ";
        if(i == 0) {
          sum += log(p);
        }
      }

      mblas::Swap(Embeddings, NextEmbeddings);
      mblas::Swap(State, NextState);
    }
    std::cerr << i << " " << sum << std::endl;
  }
}
