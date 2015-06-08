// -*- c++ -*-
// written by Ulrich Germann
#pragma once
#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "util/exception.hh"
#include "boost/format.hpp"
#include "sapt_pscore_base.h"

namespace Moses {
  namespace bitext
  {
    template<typename Token>
    class
    PScorePC : public PhraseScorer<Token>
    {
    public:
      PScorePC(string const dummy)
      {
    this->m_index = -1;
    this->m_num_feats = 1;
    this->m_feature_names.push_back(string("phrasecount"));
      }

      void
      operator()(Bitext<Token> const& bt,
         PhrasePair<Token>& pp,
		 vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	(*dest)[this->m_index] = 1;
      }
    };
  }
}
