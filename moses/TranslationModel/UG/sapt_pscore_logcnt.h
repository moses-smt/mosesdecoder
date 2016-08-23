// -*- c++ -*-
// Phrase scorer that rewards the number of phrase pair occurrences in a bitext
// with the asymptotic function x/(j+x) where x > 0 is a function
// parameter that determines the steepness of the rewards curve
// written by Ulrich Germann

#include "sapt_pscore_base.h"
#include <boost/dynamic_bitset.hpp>

namespace sapt  {

  template<typename Token>
  class
  PScoreLogCnt : public PhraseScorer<Token>
  {
    std::string m_specs;
  public:
    PScoreLogCnt(std::string const specs)
    {
      this->m_index = -1;
      this->m_specs = specs;
      if (specs.find("r1") != std::string::npos) // raw source phrase counts
	this->m_feature_names.push_back("log-r1");
      if (specs.find("s1") != std::string::npos)
	this->m_feature_names.push_back("log-s1"); // L1 sample size
      if (specs.find("g1") != std::string::npos) // coherent phrases
	this->m_feature_names.push_back("log-g1");
      if (specs.find("j") != std::string::npos) // joint counts
	this->m_feature_names.push_back("log-j");
      if (specs.find("r2") != std::string::npos) // raw target phrase counts
	this->m_feature_names.push_back("log-r2");
      this->m_num_feats = this->m_feature_names.size();
    }

    bool
    isIntegerValued(int i) const { return true; }

    void
    operator()(Bitext<Token> const& bt,
         PhrasePair<Token>& pp,
         std::vector<float> * dest = NULL) const
    {
      if (!dest) dest = &pp.fvals;
      assert(pp.raw1);
      assert(pp.sample1);
      assert(pp.good1);
      assert(pp.joint);
      assert(pp.raw2);
      size_t i = this->m_index;
      if (m_specs.find("r1") != std::string::npos)
	(*dest)[i++] = log(pp.raw1);
      if (m_specs.find("s1") != std::string::npos)
	(*dest)[i++] = log(pp.sample1);
      if (m_specs.find("g1") != std::string::npos)
	(*dest)[i++] = log(pp.good1);
      if (m_specs.find("j") != std::string::npos)
	(*dest)[i++] = log(pp.joint);
      if (m_specs.find("r2") != std::string::npos)
	(*dest)[i] = log(pp.raw2);
    }
  };
} // namespace sapt
