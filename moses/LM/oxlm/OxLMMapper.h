#pragma once

#include <map>

#include "lbl/vocabulary.h"

#include "moses/Factor.h"
#include "moses/Phrase.h"

namespace Moses
{

class OxLMMapper
{
public:
  OxLMMapper(
    const boost::shared_ptr<oxlm::Vocabulary>& vocab,
    bool pos_back_off,
    const FactorType& pos_factor_type);

  int convert(const Word& word) const;

  void convert(
    const std::vector<const Word*> &contextFactor,
    std::vector<int> &ids,
    int &word) const;

protected:
  bool posBackOff;
  FactorType posFactorType;

  typedef std::map<const Moses::Factor*, int> Coll;
  Coll moses2Oxlm;
  int kUNKNOWN;
};

} // namespace Moses
