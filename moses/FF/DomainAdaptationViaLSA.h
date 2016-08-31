// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#pragma once

#if 1
#ifdef WITH_THREADS
#include <boost/thread.hpp>
#endif
#include <string>
#include <boost/iostreams/device/mapped_file.hpp>
#include "StatelessFeatureFunction.h"
#include "moses/TranslationModel/UG/mm/tpt_tokenindex.h"
#include "moses/TranslationTask.h"
#include "moses/TypeDef.h"
#include "dalsa/LSA.h"
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>

namespace Moses
{

class DA_via_LSA : public StatelessFeatureFunction
{
public:

  struct ScopeSpecific
  {
    std::vector<float> cache;
    LsaTermMatcher match;
    SPTR<std::vector<float> > weights;
    mutable boost::shared_mutex m_lock;

    ScopeSpecific() {};
    bool init(LsaModel const* model,
              std::vector<std::string> const& context)
    {
      boost::upgrade_lock<boost::shared_mutex> rlock(m_lock);
      if (cache.size()) return false;
      boost::upgrade_to_unique_lock<boost::shared_mutex> wlock(rlock);
      match.fold_in(model,context);
      cache.assign(model->count_rows(), 2);
      return true;
    }
      
    bool init(LsaModel const* model,
              // std::vector<std::string> const& context)
              std::string const& context)
    {
      boost::upgrade_lock<boost::shared_mutex> rlock(m_lock);
      if (cache.size()) return false;
      boost::upgrade_to_unique_lock<boost::shared_mutex> wlock(rlock);
      match.fold_in(model,context);
      cache.assign(model->count_rows(), 2);
      return true;
    }

    bool init(LsaModel const* model, Moses::InputType const& snt)
    {
      boost::upgrade_lock<boost::shared_mutex> rlock(m_lock);
      if (cache.size()) return false;
      boost::upgrade_to_unique_lock<boost::shared_mutex> wlock(rlock);
      for (size_t i = 0; i < snt.GetSize(); ++i)
        match.fold_in_word(model, snt.GetWord(i).GetString(0).as_string());
      cache.assign(model->count_rows(), 2);
      return true;
    }
  };
  
protected:
  LsaModel m_model;
  std::string m_path, m_bname, m_L1, m_L2;
  size_t m_total_docs;
  
  // temporary solution while we are still at one thread per sentence
  boost::thread_specific_ptr<SPTR<ScopeSpecific> > t_scope_specific;

public:  
  DA_via_LSA(const std::string &line);

  // This FF operates over a single output factor.
  // For the time being, we hard-code that to the first factor.
  // This needs to be fixed for factored translation.
  bool 
  IsUseable(const FactorMask &mask) const 
  {
    return true;
  }
  
  void 
  EvaluateInIsolation(const Phrase &source,
                      const TargetPhrase &targetPhrase,
                      ScoreComponentCollection &scoreBreakdown,
                      ScoreComponentCollection &estimatedScores) const;

  void 
  EvaluateWithSourceContext(const InputType &input, 
                            const InputPath &inputPath,
                            const TargetPhrase &targetPhrase, 
                            const StackVec *stackVec, 
                            ScoreComponentCollection &scoreBreakdown, 
                            ScoreComponentCollection *estimatedScores) const
  {
    // std::cerr << HERE << std::endl;
    // if (targetPhrase.GetNumNonTerminals() == 0) return;

    float score = 0;
    if (*t_scope_specific)
      {
        std::vector<float>& cache = (*t_scope_specific)->cache;
        for (size_t i = 0; i < targetPhrase.GetSize(); ++i)
          {
            
            // TO DO: add operator[](StringPiece const&) to TokenIndex.
            Word const& w = targetPhrase.GetWord(i);
            uint32_t id = m_model.row2(w.GetString(0).as_string());
	  
            // TO DO: accommodate factors instead of always using factor 0
            // TO DO: maintain a mapping from Moses word ids to internal word IDs
            // will speed things up! 
            if (id < cache.size()) 
              {
                float& c = cache[id];
                if (c > 1)
                  {
                    float v = (*t_scope_specific)->match(w.GetString(0).as_string());
                    c = log((0.5 + v)/2);
                  }
                score += c;
              }
          }
      }
    scoreBreakdown.PlusEquals(this,score);
  }


  void 
  EvaluateTranslationOptionListWithSourceContext
  (const InputType &input, const TranslationOptionList &translationOptionList)
    const
  {
    // std::cerr << HERE << std::endl;
  }

  void 
  EvaluateWhenApplied
  (const Hypothesis& hypo, ScoreComponentCollection* accumulator) const
  {
    // std::cerr << HERE << std::endl;
  }

  void
  EvaluateWhenApplied
  (const ChartHypothesis &hypo, ScoreComponentCollection* accumulator) const
  {
    // std::cerr << HERE << std::endl;
  }

  void 
  Load(AllOptions::ptr const& opts);


  void 
  SetParameter(const std::string& key, const std::string& value);

  void
  InitializeForInput(ttasksptr const& ttask);
};

}

#endif
