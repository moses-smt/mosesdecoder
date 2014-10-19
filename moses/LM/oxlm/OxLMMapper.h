#pragma once

#include <map>

#include "lbl/vocabulary.h"

#include "moses/Factor.h"
#include "moses/Phrase.h"

namespace Moses {

class OxLMMapper {
 public:
  OxLMMapper(const boost::shared_ptr<oxlm::Vocabulary>& vocab);

  int convert(const Moses::Factor *factor) const;

  void convert(
      const std::vector<const Word*> &contextFactor,
      std::vector<int> &ids, int &word) const;

 private:
  typedef std::map<const Moses::Factor*, int> Coll;
  Coll moses2Oxlm;
  int kUNKNOWN;
};

}
