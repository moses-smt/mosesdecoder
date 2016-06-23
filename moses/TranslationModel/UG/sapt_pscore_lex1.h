// -*- c++ -*-
// Phrase scorer that counts the number of unaligend words in the phrase
// written by Ulrich Germann

#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "sapt_pscore_base.h"
#include <boost/dynamic_bitset.hpp>

namespace sapt
{
  template<typename Token>
  class
  PScoreLex1 : public PhraseScorer<Token>
  {
    float    m_alpha;
    std::string m_lexfile;
  public:
    sapt::LexicalPhraseScorer2<Token> scorer;

    PScoreLex1(std::string const& alphaspec, std::string const& lexfile)
    {
      this->m_index = -1;
      this->m_num_feats = 2;
      this->m_feature_names.reserve(2);
      this->m_feature_names.push_back("lexfwd");
      this->m_feature_names.push_back("lexbwd");
      m_alpha = atof(alphaspec.c_str());
      m_lexfile = lexfile;
    }

    void
    load()
    {
      scorer.open(m_lexfile);
    }

    void
    operator()(Bitext<Token> const& bt,
         PhrasePair<Token>& pp,
         std::vector<float> * dest = NULL) const
    {
      if (!dest) dest = &pp.fvals;
      // uint32_t sid1=0,sid2=0,off1=0,off2=0,len1=0,len2=0;
      // parse_pid(pp.p1, sid1, off1, len1);
      // parse_pid(pp.p2, sid2, off2, len2);
#if 0
      cout << len1 << " " << len2 << endl;
      Token const* t1 = bt.T1->sntStart(sid1);
      for (size_t i = off1; i < off1 + len1; ++i)
	cout << (*bt.V1)[t1[i].id()] << " ";
      cout << __FILE__ << ":" << __LINE__ << endl;

      Token const* t2 = bt.T2->sntStart(sid2);
      for (size_t i = off2; i < off2 + len2; ++i)
	cout << (*bt.V2)[t2[i].id()] << " ";
      cout << __FILE__ << ":" << __LINE__ << endl;

      BOOST_FOREACH (int a, pp.aln)
	cout << a << " " ;
      cout << __FILE__ << ":" << __LINE__ << "\n" << endl;

      scorer.score(bt.T1->sntStart(sid1)+off1,0,len1,
		   bt.T2->sntStart(sid2)+off2,0,len2,
		   pp.aln, m_alpha,
		   (*dest)[this->m_index],
		   (*dest)[this->m_index+1]);
#endif
      scorer.score(pp.start1,0, pp.len1,
		   pp.start2,0, pp.len2, pp.aln, m_alpha,
		   (*dest)[this->m_index],
		   (*dest)[this->m_index+1]);
    }
  };
} //namespace sapt

