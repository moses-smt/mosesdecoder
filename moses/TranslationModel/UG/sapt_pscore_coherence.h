// -*- c++ -*-
// written by Ulrich Germann
#pragma once
#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "util/exception.hh"
#include "boost/format.hpp"

namespace sapt
{
  template<typename Token>
  class
  PScoreCoherence : public PhraseScorer<Token>
  {
  public:
    PScoreCoherence(std::string const dummy)
    {
      this->m_index = -1;
      this->m_num_feats = 1;
      this->m_feature_names.push_back(std::string("coherence"));
    }

    void
    operator()(Bitext<Token> const& bt,
	       PhrasePair<Token>& pp,
	       std::vector<float> * dest = NULL) const
    {
      if (!dest) dest = &pp.fvals;
      (*dest)[this->m_index] = log(pp.good1) - log(pp.sample1);
    }
  };
}

