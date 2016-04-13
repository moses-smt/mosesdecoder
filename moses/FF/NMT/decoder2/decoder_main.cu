#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/timer/timer.hpp>
#include <boost/algorithm/string.hpp>

#include <thrust/functional.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/device_ptr.h>
#include <thrust/extrema.h>
#include <thrust/sort.h>
#include <thrust/sequence.h>


#include "mblas/matrix.h"
#include "dl4mt.h"
#include "vocab.h"

#include "states.h"

using namespace mblas;

typedef std::tuple<size_t, size_t, float> Hypothesis;
typedef std::vector<Hypothesis> Beam;

void BestHyps(Beam& bestHyps, const Beam& prevHyps, mblas::Matrix& Probs, const size_t beamSize) {
  mblas::Matrix Costs(Probs.Rows(), 1);
  thrust::host_vector<float> vCosts;
  for(const Hypothesis& h : prevHyps)
    vCosts.push_back(std::get<2>(h));
  thrust::copy(vCosts.begin(), vCosts.end(), Costs.begin());
  //mblas::debug1(Costs);
  
  mblas::BroadcastColumn(Log(_1) + _2, Probs, Costs);
  
  thrust::device_vector<unsigned> keys(Probs.size());
  thrust::sequence(keys.begin(), keys.end());
  
  // Tutaj przydalaby sie funkcja typu partition_n zamiast pelnego sort
  thrust::sort_by_key(Probs.begin(), Probs.end(), keys.begin(), thrust::greater<float>());
  
  thrust::host_vector<unsigned> bestKeys(beamSize);
  thrust::copy_n(keys.begin(), beamSize, bestKeys.begin());
  thrust::host_vector<float> bestCosts(beamSize);
  thrust::copy_n(Probs.begin(), beamSize, bestCosts.begin());
  
  for(size_t i = 0; i < beamSize; i++) {
    size_t wordIndex = bestKeys[i] % Probs.Cols();
    size_t hypIndex  = bestKeys[i] / Probs.Cols();
    float  cost = bestCosts[i];
    //std::cerr << wordIndex << " " << hypIndex << " " << cost << std::endl;
    bestHyps.emplace_back(wordIndex, hypIndex, cost);  
  }
}

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
  
  Weights weights("testmodel/model.npz", device);
  Vocab svcb("testmodel/vocab.en.txt");
  Vocab tvcb("testmodel/vocab.de.txt");
  
  std::cerr << "Creating encoder" << std::endl;
  Encoder encoder(weights);

  std::cerr << "Creating decoder" << std::endl;
  Decoder decoder(weights);
  
  mblas::Matrix State, NextState, BeamState;
  mblas::Matrix Embeddings, NextEmbeddings;
  mblas::Matrix Probs;
    
  std::string source;
  boost::timer::auto_cpu_timer timer;
  
  while(std::getline(std::cin, source)) {
    std::vector<std::string> sourceSplit;
    boost::split(sourceSplit, source, boost::is_any_of(" "),
                 boost::token_compress_on);
      
    std::vector<size_t> sourceWords(sourceSplit.size());
    std::transform(sourceSplit.begin(), sourceSplit.end(), sourceWords.begin(),
                   [&](const std::string& w) { return svcb[w]; });
    sourceWords.push_back(svcb["</s>"]);
    
    mblas::Matrix SourceContext;
    encoder.GetContext(sourceWords, SourceContext);
  
    size_t beamSize = 10;
    
    decoder.EmptyState(State, SourceContext, 1);
    decoder.EmptyEmbedding(Embeddings, 1);
    
    std::vector<Beam> history;
    
    Beam prevHyps;
    prevHyps.emplace_back(0, 0, 0.0);
    
    do {
      decoder.MakeStep(NextState, Probs, State, Embeddings, SourceContext);
      
      Beam hyps;
      BestHyps(hyps, prevHyps, Probs, beamSize);
      
      std::vector<size_t> nextWords(hyps.size());
      std::transform(hyps.begin(), hyps.end(), nextWords.begin(),
                     [](Hypothesis& h) { return std::get<0>(h); });
      decoder.Lookup(NextEmbeddings, nextWords);
      
      std::vector<size_t> beamStateIds(hyps.size());
      std::transform(hyps.begin(), hyps.end(), beamStateIds.begin(),
                     [](Hypothesis& h) { return std::get<1>(h); });
      mblas::Assemble(BeamState, NextState, beamStateIds);
      
      mblas::Swap(Embeddings, NextEmbeddings);
      mblas::Swap(State, BeamState);
      
      history.push_back(hyps);
      prevHyps.swap(hyps);
      
    } while(std::get<0>(prevHyps[0]) != tvcb["</s>"] && history.size() < sourceWords.size() * 3);
    
    std::vector<size_t> targetWords;
    size_t best = 0;
    for(auto b = history.rbegin(); b != history.rend(); b++) {
      auto& bestHyp = (*b)[best];
      targetWords.push_back(std::get<0>(bestHyp));
      best = std::get<1>(bestHyp);
    }
    
    std::reverse(targetWords.begin(), targetWords.end());
    for(size_t i = 0; i < targetWords.size(); ++i) {
      if(tvcb[targetWords[i]] != "</s>") {
        if(i > 0) {
          std::cout << " ";
        }
        std::cout << tvcb[targetWords[i]];
      }
    }
    std::cout << std::endl;
  }
}