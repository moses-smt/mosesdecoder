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
  UnknownWordPenaltyProducer(const std::string &line)
    : StatelessFeatureFunction("UnknownWordPenalty",1, line) {
    m_tuneable = false;
    CHECK(m_args.size() == 0);
  }

  bool IsUseable(const FactorMask &mask) const {
    return true;
  }

};

}

