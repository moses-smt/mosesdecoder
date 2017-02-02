// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// Phrase scorer that counts the number of unaligend words in the phrase
// written by Ulrich Germann

#include "sapt_pscore_base.h"
#include <boost/dynamic_bitset.hpp>
#include <vector>

namespace sapt
{
  template<typename Token>
  class
  PScoreUnaligned : public PhraseScorer<Token>
  {
    typedef boost::dynamic_bitset<uint64_t> bitvector;
  public:
    PScoreUnaligned(std::string const spec)
    {
      this->m_index = -1;
      int f = this->m_num_feats = atoi(spec.c_str());
      UTIL_THROW_IF2(f != 1 && f != 2,"unal parameter must be 1 or 2 at "<<HERE);
      this->m_feature_names.resize(f);
      if (f == 1)
	this->m_feature_names[0] = "unal";
      else
	{
	  this->m_feature_names[0] = "unal-s";
	  this->m_feature_names[1] = "unal-t";
	}
    }

    bool
    isLogVal(int i) const { return false; }

    bool
    isIntegerValued(int i) const { return true; }

    void
    operator()(Bitext<Token> const& bt,
         PhrasePair<Token>& pp,
         std::vector<float> * dest = NULL) const
    {
      if (!dest) dest = &pp.fvals;
      // uint32_t sid1=0,sid2=0,off1=0,off2=0,len1=0,len2=0;
      // parse_pid(pp.p1, sid1, off1, len1);
      // parse_pid(pp.p2, sid2, off2, len2);
      bitvector check1(pp.len1),check2(pp.len2);
      for (size_t i = 0; i < pp.aln.size(); )
	{
	  check1.set(pp.aln[i++]);
	  check2.set(pp.aln.at(i++));
	}

      if (this->m_num_feats == 1)
	{
	  (*dest)[this->m_index]  = pp.len1 - check1.count();
	  (*dest)[this->m_index] += pp.len2 - check2.count();
	}
      else
	{
	  (*dest)[this->m_index]   = pp.len1 - check1.count();
	  (*dest)[this->m_index+1] = pp.len2 - check2.count();
	}
    }
  };
} // namespace sapt
