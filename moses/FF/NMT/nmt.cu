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

NMT::NMT(const std::string& model,
         const std::string& src,
         const std::string& trg)
  : w_(new Weights(model)), src_(new Vocab(src)), trg_(new Vocab(trg)),
    encoder_(new Encoder(*w_)), decoder_(new Decoder(*w_))
  {}

void NMT::CalcSourceContext(const std::vector<std::string>& s) {
  std::vector<size_t> words(s.size());
  std::transform(s.begin(), s.end(), words.begin(),
                 [&](const std::string& w) { return (*src_)[w]; });
  words.push_back((*src_)["</s>"]);
  
  SourceContext.reset(new Matrix());
  Matrix& sc = *boost::static_pointer_cast<Matrix>(SourceContext);
  encoder_->GetContext(words, sc);
  debug1(sc);
}

void NMT::MakeStep(
  const std::vector<std::string>& nextWords,
  const std::vector<std::string>& lastWords,
  std::vector<WhichState>& inputStates,
  std::vector<double>& logProbs,
  std::vector<WhichState>& nextStates,
  std::vector<bool>& unks) {
  
  Matrix& sourceContext = *boost::static_pointer_cast<Matrix>(SourceContext);
  
  Matrix lastEmbeddings;
  FillEmbeddings(lastEmbeddings, lastWords, decoder_);
  
  std::vector<size_t> ids;
  Matrix nextEmbeddings;
  FillEmbeddings(nextEmbeddings, nextWords, ids, unks, decoder_);
  
  Matrix prevStates;
  ConstructPrevStates(prevStates, inputStates, states_);
  
  Matrix probs;
  Matrix alignedSourceContex;
  decoder_.GetProbs(probs, alignedSourceContext,
                    prevStates, lastEmbeddings, sourceContext);  

  for(size_t i = 0; i < ids.size(); ++j) {
    float p = Probs(i, ids[i]);
    logProbs.push_back(log(p));
  }
                    
  states_.push_back(new Matrix());
  Matrix& nextStates = *boost::static_pointer_cast<Matrix>(states.back());
  decoder_.GetNextState(nextStates, nextEmbeddings,
                        prevStates, AlignedSourceContext);
  
  size_t current = 0;
  for(size_t i = 0; i < nextStates.Rows(); ++i) {
    nextStates.push_back(Which(states_.size() - 1, i));
  }
}
