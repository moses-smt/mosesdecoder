// -*- c++ -*-
#pragma once
#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "util/exception.hh"

namespace Moses {
  namespace bitext
  {

    template<typename Token>
    class
    PhraseScorer
    {
    protected:
      int m_index;
      int m_num_feats;
      vector<string> m_feature_names;
    public:
 
      virtual 
      void 
      operator()(Bitext<Token> const& pt, PhrasePair& pp, vector<float> * dest=NULL) 
	const = 0;
    
      int 
      fcnt() const 
      { return m_num_feats; }
    
      vector<string> const &
      fnames() const
      { return m_feature_names; }

      string const &
      fname(int i) const
      { 
	UTIL_THROW_IF2((i < m_index || i >= m_index + m_num_feats),
		       "Feature name index out of range at " 
		       << __FILE__ << ":" << __LINE__);
	return m_feature_names.at(i - m_index); 
      }
    
      int 
      getIndex() const 
      { return m_index; }
    };
  
    ////////////////////////////////////////////////////////////////////////////////
  
    template<typename Token>
    class
    PScorePfwd : public PhraseScorer<Token>
    {
      float conf;
      char denom;
    public:
      PScorePfwd() 
      {
	this->m_num_feats = 1;
      }

      int 
      init(int const i, float const c, char d) 
      { 
	conf  = c; 
	denom = d;
	this->m_index = i;
	ostringstream buf;
	buf << format("pfwd-%c%.3f") % denom % c;
	this->m_feature_names.push_back(buf.str());
	return i + this->m_num_feats;
      }

      void 
      operator()(Bitext<Token> const& bt, PhrasePair & pp, 
		 vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	if (pp.joint > pp.good1) 
	  {
	    cerr<<bt.toString(pp.p1,0)<<" ::: "<<bt.toString(pp.p2,1)<<endl;
	    cerr<<pp.joint<<"/"<<pp.good1<<"/"<<pp.raw2<<endl;
	  }
	switch (denom)
	  {
	  case 'g': 
	    (*dest)[this->m_index] = log(lbop(pp.good1, pp.joint, conf)); 
	    break;
	  case 's': 
	    (*dest)[this->m_index] = log(lbop(pp.sample1, pp.joint, conf)); 
	    break;
	  case 'r':
	    (*dest)[this->m_index] = log(lbop(pp.raw1, pp.joint, conf)); 
	  }
      }
    };
  
    ////////////////////////////////////////////////////////////////////////////////

    template<typename Token>
    class
    PScorePbwd : public PhraseScorer<Token>
    {
      float conf;
      char denom;
    public:
      PScorePbwd() 
      {
	this->m_num_feats = 1;
      }

      int 
      init(int const i, float const c, char d) 
      { 
	conf = c; 
	denom = d;
	this->m_index = i;
	ostringstream buf;
	buf << format("pbwd-%c%.3f") % denom % c;
	this->m_feature_names.push_back(buf.str());
	return i + this->m_num_feats;
      }

      void 
      operator()(Bitext<Token> const& bt, PhrasePair& pp, 
		 vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	// we use the denominator specification to scale the raw counts on the 
	// target side; the clean way would be to counter-sample
	uint32_t r2 = pp.raw2;
	if      (denom == 'g') r2 = round(r2 * float(pp.good1)   / pp.raw1);
	else if (denom == 's') r2 = round(r2 * float(pp.sample1) / pp.raw1);
	(*dest)[this->m_index] = log(lbop(max(r2, pp.joint),pp.joint,conf));
      }
    };
  
    ////////////////////////////////////////////////////////////////////////////////

    template<typename Token>
    class
    PScoreCoherence : public PhraseScorer<Token>
    {
    public:
      PScoreCoherence() 
      {
	this->m_num_feats = 1;
      }
    
      int 
      init(int const i) 
      { 
	this->m_index = i;
	this->m_feature_names.push_back(string("coherence"));
	return i + this->m_num_feats;
      }

      void 
      operator()(Bitext<Token> const& bt, PhrasePair& pp, 
		 vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	(*dest)[this->m_index] = log(pp.good1) - log(pp.sample1);
      }
    };
  
    ////////////////////////////////////////////////////////////////////////////////

    template<typename Token>
    class
    PScoreLogCounts : public PhraseScorer<Token>
    {
      float conf;
    public:
      PScoreLogCounts() 
      {
	this->m_num_feats = 5;
      }
    
      int 
      init(int const i) 
      { 
	this->m_index = i;
	this->m_feature_names.push_back("log-r1");
	this->m_feature_names.push_back("log-s1");
	this->m_feature_names.push_back("log-g1");
	this->m_feature_names.push_back("log-j");
	this->m_feature_names.push_back("log-r2");
	return i + this->m_num_feats;
      }
    
      void 
      operator()(Bitext<Token> const& bt, PhrasePair& pp, 
		 vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	size_t i = this->m_index;
	assert(pp.raw1);
	assert(pp.sample1);
	assert(pp.good1);
	assert(pp.joint);
	assert(pp.raw2);
	(*dest)[i]   = -log(pp.raw1);
	(*dest)[++i] = -log(pp.sample1);
	(*dest)[++i] = -log(pp.good1);
	(*dest)[++i] = +log(pp.joint);
	(*dest)[++i] = -log(pp.raw2);
      }
    };
  
    template<typename Token>
    class
    PScoreLex : public PhraseScorer<Token>
    {
      float const m_alpha;
    public:
      LexicalPhraseScorer2<Token> scorer;
    
      PScoreLex(float const a) 
	: m_alpha(a) 
      { this->m_num_feats = 2; }
    
      int 
      init(int const i, string const& fname) 
      { 
	scorer.open(fname); 
	this->m_index = i;
	this->m_feature_names.push_back("lexfwd");
	this->m_feature_names.push_back("lexbwd");
	return i + this->m_num_feats;
      }
    
      void 
      operator()(Bitext<Token> const& bt, PhrasePair& pp, vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	uint32_t sid1=0,sid2=0,off1=0,off2=0,len1=0,len2=0;
	parse_pid(pp.p1, sid1, off1, len1);
	parse_pid(pp.p2, sid2, off2, len2);
	
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
	
#endif
	scorer.score(bt.T1->sntStart(sid1)+off1,0,len1,
		     bt.T2->sntStart(sid2)+off2,0,len2,
		     pp.aln, m_alpha,
		     (*dest)[this->m_index],
		     (*dest)[this->m_index+1]);
      }
      
    };
  
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
      operator()(Bitext<Token> const& bt, PhrasePair& pp, vector<float> * dest = NULL) const
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
      operator()(Bitext<Token> const& bt, PhrasePair& pp, vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	(*dest)[this->m_index] = 1;
      }
    
    };
  }
}
