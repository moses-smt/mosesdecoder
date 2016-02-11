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
#include "states.h"

using namespace mblas;

NMT::NMT(const boost::shared_ptr<Weights> model,
         const boost::shared_ptr<Vocab> src,
         const boost::shared_ptr<Vocab> trg)
  : w_(model), src_(src), trg_(trg),
    encoder_(new Encoder(*w_)), decoder_(new Decoder(*w_)),
    states_(new States()), firstWord_(true)
  { }

size_t NMT::GetDevices(size_t maxDevices) {
  int num_gpus = 0;   // number of CUDA GPUs
  cudaGetDeviceCount(&num_gpus);
  std::cerr << "Number of CUDA devices: " << num_gpus << std::endl;
  
  for (int i = 0; i < num_gpus; i++) {
      cudaDeviceProp dprop;
      cudaGetDeviceProperties(&dprop, i);
      std::cerr << i << ": " << dprop.name << std::endl;
  }
  return (size_t)std::min(num_gpus, (int)maxDevices);
}

void NMT::SetDevice() {
  cudaSetDevice(w_->GetDevice());
}

size_t NMT::GetDevice() {
  return w_->GetDevice();
}

void NMT::ClearStates() { 
  states_->Clear();
}

boost::shared_ptr<Weights> NMT::NewModel(const std::string& path, size_t device) {
  cudaSetDevice(device);
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
  
  SourceContext_.reset(new Matrix());
  Matrix& SC = *boost::static_pointer_cast<Matrix>(SourceContext_);
  encoder_->GetContext(words, SC);
}

StateInfoPtr NMT::EmptyState() {
  Matrix& SC = *boost::static_pointer_cast<Matrix>(SourceContext_);
  Matrix Empty;
  decoder_->EmptyState(Empty, SC, 1);
  std::vector<StateInfoPtr> infos;
  states_->SaveStates(infos, Empty);
  return infos.back();
}

void NMT::MakeStep(
  const std::vector<std::string>& nextWords,
  const std::vector<std::string>& lastWords,
  std::vector<StateInfoPtr>& inputStates,
  std::vector<double>& logProbs,
  std::vector<StateInfoPtr>& outputStates,
  std::vector<bool>& unks) {
  
  Matrix& sourceContext = *boost::static_pointer_cast<Matrix>(SourceContext_);
  
  Matrix lastEmbeddings;
  if(firstWord_) {
    firstWord_ = false;
    // Only empty state in state cache, so this is the first word
    decoder_->EmptyEmbedding(lastEmbeddings, lastWords.size());
  }
  else {
    // Not the first word
    std::vector<size_t> lastIds(lastWords.size());
    std::transform(lastWords.begin(), lastWords.end(), lastIds.begin(),
                   [&](const std::string& w) { return (*trg_)[w]; });
    decoder_->Lookup(lastEmbeddings, lastIds);
  }
  
  Matrix nextEmbeddings;
  std::vector<size_t> nextIds(nextWords.size());
  std::transform(nextWords.begin(), nextWords.end(), nextIds.begin(),
                 [&](const std::string& w) { return (*trg_)[w]; });
  decoder_->Lookup(nextEmbeddings, nextIds);
  for(auto id : nextIds) {
    if(id != 1)
      unks.push_back(true);
    else
      unks.push_back(false);
  }
  
  Matrix prevStates;
  states_->ConstructStates(prevStates, inputStates);

  Matrix probs;
  Matrix alignedSourceContext;
  decoder_->GetProbs(probs, alignedSourceContext,
                     prevStates, lastEmbeddings, sourceContext);  
  
  for(size_t i = 0; i < nextIds.size(); ++i) {
    float p = probs(i, nextIds[i]);
    logProbs.push_back(log(p));
  }
                    
  Matrix nextStates;
  decoder_->GetNextState(nextStates, nextEmbeddings,
                        prevStates, alignedSourceContext);
  states_->SaveStates(outputStates, nextStates);
  
}
