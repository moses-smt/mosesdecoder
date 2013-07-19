#pragma once

// $Id$

#include "StatelessFeatureFunction.h"
#include "util/check.hh"

namespace Moses
{

class WordsRange;


/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction
{
public:
  UnknownWordPenaltyProducer(const std::string &line);

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }
  std::vector<float> DefaultWeights() const;

};

}

