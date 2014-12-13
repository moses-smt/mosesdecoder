// -*- c++ -*-
// written by Ulrich Germann 
#pragma once
#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "util/exception.hh"
#include "boost/format.hpp"
#include "boost/foreach.hpp"

namespace Moses {
  namespace bitext
  {
    template<typename Token>
    class
    PScorePfwd : public PhraseScorer<Token>
    {
      float   conf;
      string denom;

    public:

      PScorePfwd(float const c, string d) 
      { 
	this->m_index = -1;
	conf  = c; 
	denom = d;
	size_t checksum = d.size();
	BOOST_FOREACH(char const& x, denom)
	  {
	    if (x == '+') { --checksum; continue; }
	    if (x != 'g' && x != 's' && x != 'r') continue;
	    string s = (format("pfwd-%c%.3f") % x % c).str();
	    this->m_feature_names.push_back(s);
	  }
	this->m_num_feats = this->m_feature_names.size();
	UTIL_THROW_IF2(this->m_feature_names.size() != checksum, 
		       "Unknown parameter in specification '" 
		       << d << "' for Pfwd phrase scorer at " << HERE);
      }
      
      void 
      operator()(Bitext<Token> const& bt, PhrasePair<Token> & pp, 
		 vector<float> * dest = NULL) const
      {
	if (!dest) dest = &pp.fvals;
	if (pp.joint > pp.good1) 
	  {
	    pp.joint = pp.good1;
	    // cerr<<bt.toString(pp.p1,0)<<" ::: "<<bt.toString(pp.p2,1)<<endl;
	    // cerr<<pp.joint<<"/"<<pp.good1<<"/"<<pp.raw2<<endl;
	  }
	size_t i = this->m_index;
	BOOST_FOREACH(char const& c, this->denom)
	  {
	    switch (c)
	      {
	      case 'g': 
		(*dest)[i++] = log(lbop(pp.good1, pp.joint, conf)); 
		break;
	      case 's': 
		(*dest)[i++] = log(lbop(pp.sample1, pp.joint, conf)); 
		break;
	      case 'r':
		(*dest)[i++] = log(lbop(pp.raw1, pp.joint, conf)); 
	      }
	  }
      }
    };
  }
}
  
