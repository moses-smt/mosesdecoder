// -*- c++ -*-
// written by Ulrich Germann 
#pragma once
#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "util/exception.hh"
#include "boost/format.hpp"
#include "sapt_pscore_base.h"

// DEPRECATED CODE: Word and phrase penalties are now 
// added by the decoder.

namespace Moses {
  namespace bitext
  {
    /// Word penalty
    template<typename Token>
    class
    PScoreWP : public PhraseScorer<Token>
    {
    public:
    
      PScoreWP() { this->m_num_feats = 1; }
    
      int 
      init(int const i) 
      {
	this->m_index = i;
	return i + this->m_num_feats;
      }
    
      void 
      operator()(Bitext<Token> const& bt, PhrasePair<Token>& pp, 
		 vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	uint32_t sid2=0,off2=0,len2=0;
	parse_pid(pp.p2, sid2, off2, len2);
	(*dest)[this->m_index] = len2;
      }
    
    };
  
    /// Phrase penalty
    template<typename Token>
    class
    PScorePP : public PhraseScorer<Token>
    {
    public:
    
      PScorePP() { this->m_num_feats = 1; }
    
      int 
      init(int const i) 
      {
	this->m_index = i;
	return i + this->m_num_feats;
      }
    
      void 
      operator()(Bitext<Token> const& bt, PhrasePair<Token>& pp, 
		 vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	(*dest)[this->m_index] = 1;
      }
    
    };
  }
}
