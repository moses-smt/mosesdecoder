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
class TranslationOptionCollection;

/** Functions and variables you need to decoder an input using the
 *  phrase-based decoder (NO cube-pruning)
 *  Instantiated by the Manager class
 */
class SearchNormal: public Search
{
protected:
  //! stacks to store hypotheses (partial translations)
  // no of elements = no of words in source + 1
  std::vector < HypothesisStack* > m_hypoStackColl;

  /** actual (full expanded) stack of hypotheses*/
  HypothesisStackNormal* actual_hypoStack;

  /** pre-computed list of translation options for the phrases in this sentence */
  const TranslationOptionCollection &m_transOptColl;

  // functions for creating hypotheses

  virtual bool
  ProcessOneStack(HypothesisStack* hstack);

  virtual void
  ProcessOneHypothesis(const Hypothesis &hypothesis);

  virtual void
  ExpandAllHypotheses(const Hypothesis &hypothesis, size_t startPos, size_t endPos);

  virtual void
  ExpandHypothesis(const Hypothesis &hypothesis,
                   const TranslationOption &transOpt,
                   float expectedScore,
                   float estimatedScore,
                   const Bitmap &bitmap);

public:
  SearchNormal(Manager& manager, const TranslationOptionCollection &transOptColl);
  ~SearchNormal();

  void Decode();

  void OutputHypoStackSize();
  void OutputHypoStack();

  virtual const std::vector < HypothesisStack* >& GetHypothesisStacks() const;
  virtual const Hypothesis *GetBestHypothesis() const;
};

}

#endif
