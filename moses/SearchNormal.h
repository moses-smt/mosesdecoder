#ifndef moses_SearchNormal_h
#define moses_SearchNormal_h

#include <vector>
#include "Search.h"
#include "HypothesisStackNormal.h"
#include "TranslationOptionCollection.h"
#include "Timer.h"

namespace Moses
{

class Manager;
class InputType;
class TranslationOptionCollection;

class SearchNormal;

class FunctorNormal {
  public:
    FunctorNormal(SearchNormal* search) : m_search(search) {}
    
    virtual void operator()(const Hypothesis &hypothesis,
                       size_t startPos, size_t endPos) = 0;
    
    virtual SearchNormal* GetSearch() {
      return m_search;
    }
    
  protected:
    SearchNormal* m_search;
};

class ExpanderNormal : public FunctorNormal {
  public:
    ExpanderNormal(SearchNormal* search) : FunctorNormal(search) {}
    virtual void operator()(const Hypothesis &hypothesis,
                       size_t startPos, size_t endPos);
};

class CollectorNormal : public FunctorNormal, public Collector {
  public:
    CollectorNormal(SearchNormal* search) : FunctorNormal(search) {}
    virtual void operator()(const Hypothesis &hypothesis,
                       size_t startPos, size_t endPos);

    std::vector<const Hypothesis*> GetHypotheses() {
      return m_hypotheses;                   
    }
    
    std::vector<const TranslationOptionList*>& GetOptions(int hypId) {
      return m_options[hypId];
    }
    
  private:
    std::vector<const Hypothesis*> m_hypotheses;
    std::map<size_t, std::vector<const TranslationOptionList*> > m_options;
};

/** Functions and variables you need to decoder an input using the
 *  phrase-based decoder (NO cube-pruning)
 *  Instantiated by the Manager class
 */
class SearchNormal: public Search
{
protected:
  friend ExpanderNormal;
  friend CollectorNormal;
    
  const InputType &m_source;
  //! stacks to store hypotheses (partial translations)
  // no of elements = no of words in source + 1
  std::vector < HypothesisStack* > m_hypoStackColl;

  /** actual (full expanded) stack of hypotheses*/
  HypothesisStackNormal* actual_hypoStack;

  /** pre-computed list of translation options for the phrases in this sentence */
  const TranslationOptionCollection &m_transOptColl;

  // functions for creating hypotheses

  void CacheForNeural(Collector& collector);
  
  virtual bool
  ProcessOneStack(HypothesisStack* hstack, FunctorNormal* functor);

  virtual void
  ProcessOneHypothesis(const Hypothesis &hypothesis, FunctorNormal* functor);

  virtual void
  ExpandAllHypotheses(const Hypothesis &hypothesis, size_t startPos, size_t endPos);

  virtual void
  ExpandHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt,
                   float expectedScore);

public:
  SearchNormal(Manager& manager, const InputType &source, const TranslationOptionCollection &transOptColl);
  ~SearchNormal();

  void Decode();

  void OutputHypoStackSize();
  void OutputHypoStack();

  virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const;
  virtual const Hypothesis *GetBestHypothesis() const;
};

}

#endif
