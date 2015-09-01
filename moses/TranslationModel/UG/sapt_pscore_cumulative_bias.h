// -*- c++ -*-
// Phrase scorer that records the aggregated bias score 
// 

#include "sapt_pscore_base.h"
#include <boost/dynamic_bitset.hpp>

namespace sapt  {
  
  template<typename Token>
  class
  PScoreCumBias : public PhraseScorer<Token>
  {
  public:
    PScoreCumBias(std::string const spec)
    {
      this->m_index = -1;
      this->m_feature_names.push_back("cumb");
      this->m_num_feats = this->m_feature_names.size();
    }

    bool
    isIntegerValued(int i) const { return false; }
    
    void
    operator()(Bitext<Token> const& bt,
	       PhrasePair<Token>& pp,
	       std::vector<float> * dest = NULL) const
    {
      if (!dest) dest = &pp.fvals;
      (*dest)[this->m_index] = log(pp.cum_bias);
    }
  };
} // namespace sapt

