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