// -*- mode: c++; indent-tabs-mode: nil; tab-width: 2 -*-
// Phrase scorer that considers the length ratio of the two phrases.
// Written by Ulrich Germann.
// 
// Phrase pair generation is modeled as a bernully experiment with a biased coin:
// heads: produce a word in L1, tails: produce a word in L2
// total number of coin tosses: len(phrase 1) + len(phrase 2)
// probability p(w from L1) = length(corpus 1) / (length(corpus 1) + length(corpus 2)
#pragma once

#include "sapt_pscore_base.h"
#include <boost/dynamic_bitset.hpp>
#include <boost/math/distributions/binomial.hpp>
#include "mm/ug_ttrack_base.h"

namespace sapt  {
    
    
  // // return the probability that a phrase length ratio is as extrem as
  // // or more extreme as alen:blen. Based on a binomial experiment with 
  // // (alen + blen) trials and the probability of producing ratio L1 tokens per
  // // L2 token
  // float 
  // length_ratio_prob(float const alen, float const blen, float const ratio)
  // {
  //   if (alen + blen == 0) return 1;
  //   float p = 1./(1 + ratio);
  //   boost::math::binomial bino(alen+blen,p);
  //   if (blen/(alen+blen) < p)
  // 	return cdf(bino,blen); 
  //   else
  // 	return cdf(complement(bino,blen - 1));
  // }

  template<typename Token>
  class
  PScoreLengthRatio : public PhraseScorer<Token>
  {
  public:
    PScoreLengthRatio(std::string const& spec)
    {
      this->m_feature_names.push_back("lenrat");
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
      float p  = float(bt.T1->numTokens());
      p /= bt.T1->numTokens() + bt.T2->numTokens();
      float len1 = sapt::len_from_pid(pp.p1);
      float len2 = sapt::len_from_pid(pp.p2);
	
      boost::math::binomial binomi(len1 + len2, p);
      float& x = (*dest)[this->m_index];
      if (len2/(len1 + len2) < p)
        x = log(boost::math::cdf(binomi,len2)); 
      else
        x = log(boost::math::cdf(boost::math::complement(binomi,len2 - 1)));
    }
  };
} // namespace sapt

