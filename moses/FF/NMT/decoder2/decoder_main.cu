#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <limits> 
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

using namespace mblas;

typedef std::tuple<size_t, size_t, float> Hypothesis;
typedef std::vector<Hypothesis> Beam;
typedef std::vector<Beam> History;

void BestHyps(Beam& bestHyps, const Beam& prevHyps, mblas::Matrix& Probs, const size_t beamSize) {
  mblas::Matrix Costs(Probs.Rows(), 1);
  thrust::host_vector<float> vCosts;
  for(const Hypothesis& h : prevHyps)
    vCosts.push_back(std::get<2>(h));
  thrust::copy(vCosts.begin(), vCosts.end(), Costs.begin());
  
  mblas::BroadcastVecColumn(Log(_1) + _2, Probs, Costs);
  
  thrust::device_vector<unsigned> keys(Probs.size());
  thrust::sequence(keys.begin(), keys.end());
  
  // Here it would be nice to have a partial sort instead of full sort
  thrust::sort_by_key(Probs.begin(), Probs.end(),
                      keys.begin(), thrust::greater<float>());
  
  thrust::host_vector<unsigned> bestKeys(beamSize);
  thrust::copy_n(keys.begin(), beamSize, bestKeys.begin());
  thrust::host_vector<float> bestCosts(beamSize);
  thrust::copy_n(Probs.begin(), beamSize, bestCosts.begin());
  
  for(size_t i = 0; i < beamSize; i++) {
    size_t wordIndex = bestKeys[i] % Probs.Cols();
    size_t hypIndex  = bestKeys[i] / Probs.Cols();
    float  cost = bestCosts[i];
    bestHyps.emplace_back(wordIndex, hypIndex, cost);  
  }
}

void FindBest(const History& history, const Vocab& vcb) {
  std::vector<size_t> targetWords;
  
  size_t best = 0;
  size_t beamSize = 0;
  float bestCost = std::numeric_limits<float>::lowest();
      
  for(auto b = history.rbegin(); b != history.rend(); b++) {
    if(b->size() > beamSize) {
      beamSize = b->size();
      for(size_t i = 0; i < beamSize; ++i) {
        if(b == history.rbegin() || std::get<0>((*b)[i]) == vcb["</s>"]) {
          if(std::get<2>((*b)[i]) > bestCost) {
            best = i;
            bestCost = std::get<2>((*b)[i]);
            targetWords.clear();
          }
        }
      }
    }
    
    auto& bestHyp = (*b)[best];
    targetWords.push_back(std::get<0>(bestHyp));
    best = std::get<1>(bestHyp);
  }

  std::reverse(targetWords.begin(), targetWords.end());
  for(size_t i = 0; i < targetWords.size(); ++i) {
    if(vcb[targetWords[i]] != "</s>") {
      if(i > 0) {
        std::cout << " ";
      }
      std::cout << vcb[targetWords[i]];
    }
  }
  std::cout << std::endl;
}

int main(int argc, char** argv) {
  size_t device = 0;
  
  if(argc > 1) {
    if(std::string(argv[1]) == "1")
      device = 1;
    else if(std::string(argv[1]) == "2")
      device = 2;
  }
  
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
  
    size_t beamSize = 12;
    
    decoder.EmptyState(State, SourceContext, 1);
    decoder.EmptyEmbedding(Embeddings, 1);
    
    History history;
    
    Beam prevHyps;
    prevHyps.emplace_back(0, 0, 0.0);
    
    do {
      decoder.MakeStep(NextState, Probs, State, Embeddings, SourceContext);
      
      Beam hyps;
      BestHyps(hyps, prevHyps, Probs, beamSize);
      history.push_back(hyps);
      
      Beam survivors;
      std::vector<size_t> beamWords;
      std::vector<size_t> beamStateIds;
      for(auto& h : hyps) {
        if(std::get<0>(h) != tvcb["</s>"]) {
          survivors.push_back(h);
          beamWords.push_back(std::get<0>(h));
          beamStateIds.push_back(std::get<1>(h));
        }
      }
      beamSize = survivors.size();
      
      if(beamSize == 0)
        break;
      
      decoder.Lookup(NextEmbeddings, beamWords);
      mblas::Assemble(BeamState, NextState, beamStateIds);
      
      mblas::Swap(Embeddings, NextEmbeddings);
      mblas::Swap(State, BeamState);
      prevHyps.swap(survivors);
      
    } while(history.size() < sourceWords.size() * 3);
    
    FindBest(history, tvcb);
  }
}