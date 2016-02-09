#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <string>
#include <boost/timer/timer.hpp>

#include "nmt.h"
#include "mblas/matrix.h"
#include "model.h"
#include "encoder.h"
#include "decoder.h"
#include "vocab.h"

using namespace mblas;

NMT::NMT(const boost::shared_ptr<Weights> model,
         const boost::shared_ptr<Vocab> src,
         const boost::shared_ptr<Vocab> trg)
  : w_(model), src_(src), trg_(trg),
    encoder_(new Encoder(*w_)), decoder_(new Decoder(*w_))
  { }

size_t NMT::GetDevices() {
  int num_gpus = 0;   // number of CUDA GPUs
  cudaGetDeviceCount(&num_gpus);
  std::cerr << "Number of CUDA devices: " << num_gpus << std::endl;

  for (int i = 0; i < num_gpus; i++) {
      cudaDeviceProp dprop;
      cudaGetDeviceProperties(&dprop, i);
      std::cerr << i << ": " << dprop.name << std::endl;
  }
  return (size_t)num_gpus;
}

void NMT::SetDevice() {
  cudaSetDevice(w_->GetDevice());
}

void NMT::ClearStates() { 
  std::vector<boost::shared_ptr<mblas::BaseMatrix> > temp;
  states_.swap(temp);
}

boost::shared_ptr<Weights> NMT::NewModel(const std::string& path, size_t device) {
  boost::shared_ptr<Weights> weights(new Weights(path, device));
  return weights;
}

boost::shared_ptr<Vocab> NMT::NewVocab(const std::string& path) {
  boost::shared_ptr<Vocab> vocab(new Vocab(path));
  return vocab;
}

void NMT::CalcSourceContext(const std::vector<std::string>& s) {  
  std::vector<size_t> words(s.size());
  std::transform(s.begin(), s.end(), words.begin(),
                 [&](const std::string& w) { return (*src_)[w]; });
  words.push_back((*src_)["</s>"]);
  
  SourceContext.reset(new Matrix());
  Matrix& sc = *boost::static_pointer_cast<Matrix>(SourceContext);
  encoder_->GetContext(words, sc);
  
  // Put empty decoder state into state cache
  states_.emplace_back(new Matrix());
  Matrix& firstStates = *boost::static_pointer_cast<Matrix>(states_.back());
  decoder_->EmptyState(firstStates, sc, 1);
}

void ConstructPrevStates(Matrix& States,
                         const std::vector<WhichState>& inputStates,
                         const std::vector<boost::shared_ptr<mblas::BaseMatrix> >& states) {
  for(auto w: inputStates) {
    //std::cerr << w.stateId << " " << w.rowNo << std::endl;
    Matrix& State = *boost::static_pointer_cast<Matrix>(states[w.stateId]);
    // @TODO: do that with preallocation
    AppendRow(States, State, w.rowNo);
  }
}

void NMT::MakeStep(
  const std::vector<std::string>& nextWords,
  const std::vector<std::string>& lastWords,
  std::vector<WhichState>& inputStates,
  std::vector<double>& logProbs,
  std::vector<WhichState>& outputStates,
  std::vector<bool>& unks) {
  
  Matrix& sourceContext = *boost::static_pointer_cast<Matrix>(SourceContext);
  
  Matrix lastEmbeddings;
  if(states_.size() > 1) {
    // Not the first word
    std::vector<size_t> lastIds(lastWords.size());
    std::transform(lastWords.begin(), lastWords.end(), lastIds.begin(),
                   [&](const std::string& w) { return (*trg_)[w]; });
    decoder_->Lookup(lastEmbeddings, lastIds);
  }
  else {
    // Only empty state in state cache, so this is the first word
    decoder_->EmptyEmbedding(lastEmbeddings, lastWords.size());
  }
  
  Matrix nextEmbeddings;
  std::vector<size_t> nextIds(nextWords.size());
  std::transform(nextWords.begin(), nextWords.end(), nextIds.begin(),
                 [&](const std::string& w) { return (*trg_)[w]; });
  decoder_->Lookup(nextEmbeddings, nextIds);
  for(auto id : nextIds) {
    if(id == 1)
      unks.push_back(true);
    else
      unks.push_back(false);
  }
  
  Matrix prevStates;
  ConstructPrevStates(prevStates, inputStates, states_);

  Matrix probs;
  Matrix alignedSourceContext;
  decoder_->GetProbs(probs, alignedSourceContext,
                     prevStates, lastEmbeddings, sourceContext);  
  
  for(size_t i = 0; i < nextIds.size(); ++i) {
    float p = probs(i, nextIds[i]);
    logProbs.push_back(log(p));
  }
                    
  states_.emplace_back(new Matrix());
  Matrix& nextStates = *boost::static_pointer_cast<Matrix>(states_.back());
  decoder_->GetNextState(nextStates, nextEmbeddings,
                        prevStates, alignedSourceContext);
  
  size_t current = states_.size() - 1;
  for(size_t i = 0; i < nextStates.Rows(); ++i) {
    outputStates.push_back(WhichState(current, i));
  }
}
