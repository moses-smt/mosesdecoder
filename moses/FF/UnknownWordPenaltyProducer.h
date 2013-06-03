#pragma once

// $Id$

#include "StatelessFeatureFunction.h"

namespace Moses
{

class WordsRange;


/** unknown word penalty */
class UnknownWordPenaltyProducer : public StatelessFeatureFunction
{
public:
  UnknownWordPenaltyProducer(const std::string &line)
    : StatelessFeatureFunction("UnknownWordPenalty",1, line) {
    m_tuneable = false;
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

};

}

