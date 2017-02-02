// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
// Base classes for suffix array-based phrase scorers
// written by Ulrich Germann
#pragma once
#include "moses/TranslationModel/UG/mm/ug_bitext.h"
#include "util/exception.hh"
#include "boost/format.hpp"

// namespace Moses {
  namespace sapt
  {

    // abstract base class that defines the common API for phrase scorers
    template<typename Token>
    class
    PhraseScorer
    {
    protected:
      int m_index;
      int m_num_feats;
      std::string m_tag;
      std::vector<std::string> m_feature_names;
    public:


      virtual ~PhraseScorer() {}

      virtual void
      operator()(Bitext<Token> const& pt, PhrasePair<Token>& pp,
                 std::vector<float> * dest=NULL) const = 0;

      void
      setIndex(int const i) { m_index = i; }

      int
      getIndex() const { return m_index; }

      int
      fcnt() const { return m_num_feats; }

      std::vector<std::string> const &
      fnames() const { return m_feature_names; }

      std::string const &
      fname(int i) const
      {
	if (i < 0) i += m_num_feats;
	UTIL_THROW_IF2(i < 0 || i >= m_num_feats,
		       "Feature name index out of range at " << HERE);
	return m_feature_names.at(i);
      }

      virtual
      bool
      isLogVal(int i) const  { return true; };
      // is this feature log valued?

      virtual
      bool
      isIntegerValued(int i) const  { return false; };
      // is this feature integer valued (e.g., count features)?

      virtual
      bool
      allowPooling() const { return true; }
      // does this feature function allow pooling of counts if
      // there are no occurrences in the respective corpus?

      virtual
      void
      load() { }

    };

    // base class for 'families' of phrase scorers that have a single
    template<typename Token>
    class
    SingleRealValuedParameterPhraseScorerFamily
      : public PhraseScorer<Token>
    {
    protected:
      std::vector<float> m_x;

      virtual
      void
      init(std::string const specs)
      {
	UTIL_THROW_IF2(this->m_tag.size() == 0,
		       "m_tag must be initialized in constructor");
	UTIL_THROW_IF2(specs.size() == 0,"empty specification string!");
	UTIL_THROW_IF2(this->m_feature_names.size(),
		       "PhraseScorer can only be initialized once!");
	this->m_index = -1;
	float x; char c;
	for (std::istringstream buf(specs); buf>>x; buf>>c)
	  {
	    this->m_x.push_back(x);
	    std::string fname = (boost::format("%s-%.2f") % this->m_tag % x).str();
	    this->m_feature_names.push_back(fname);
	  }
	this->m_num_feats = this->m_x.size();
      }
    };
  } // namespace sapt
// } // namespace moses
