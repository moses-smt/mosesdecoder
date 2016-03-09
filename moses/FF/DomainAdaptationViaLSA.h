// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif
#include <string>
#include <boost/iostreams/device/mapped_file.hpp>
#include "StatelessFeatureFunction.h"
#include "moses/TranslationModel/UG/mm/tpt_tokenindex.h"

namespace Moses
{

class DA_via_LSA : public StatelessFeatureFunction
{
public:

  struct ScopeSpecific
  {
    vector<float> weights;
    vector<float> cache;
  };
  
protected:
  uint64_t m_num_term_vectors;   // rows of the matrix
  uint32_t m_num_lsa_dimensions; // columns of the matrix

  std::string m_vocab_file;
  std::string m_term_vector_file;

  boost::iostreams::mapped_file_source m_term_vectors;
  sapt::TokenIndex m_V;
  float const* m_tvec; // pointer to T[0][0]

  // temporary solution while we are still at one thread per sentence
  boost::thread_specific_ptr<SPTR<ScopeSpecific> > t_scope_specific;

public:  
  DA_via_LSA(const std::string &line);

  // for the time being; this needs to be fixed for factored translation
  // obviously, this FF is supposed to operate over a single output factor
  // For the time being, we hard-code that to the first factor.
  bool 
  IsUseable(const FactorMask &mask) const 
  {
    return true;
  }
  
  void 
  EvaluateInIsolation(const Phrase &source,
		      const TargetPhrase &targetPhrase.
		      ScoreComponentCollection &scoreBreakdown,
		      ScoreComponentCollection &estimatedScores) const;

  void Load(AllOptions::ptr const& opts);


  void 
  SetParameter(const std::string& key, const std::string& value);

  void
  InitializeForInput(ttasksptr const& ttask);
};

}

