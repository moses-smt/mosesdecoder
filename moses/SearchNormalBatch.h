#ifndef moses_SearchNormalBatch_h
#define moses_SearchNormalBatch_h

#include "SearchNormal.h"
#include "SentenceStats.h"

namespace Moses
{

class Manager;
class InputType;
class TranslationOptionCollection;

/** Implements the phrase-based stack decoding algorithm (no cube pruning) with a twist...
 *  Language model requests are batched together, duplicate requests are removed, and requests are sent together.
 *  Useful for distributed LM where network latency is an issue.
 */
class SearchNormalBatch: public SearchNormal
{
protected:

  // Added for asynclm decoding.
  std::vector<const StatelessFeatureFunction*> m_stateless_ffs;
  std::map<int, LanguageModel*> m_dlm_ffs;
  std::map<int, StatefulFeatureFunction*> m_stateful_ffs;
  std::vector<Hypothesis*> m_partial_hypos;
  uint32_t m_batch_size;
  int m_max_stack_size;

  // functions for creating hypotheses
  void ExpandHypothesis(const Hypothesis &hypothesis,const TranslationOption &transOpt, float expectedScore);
  void EvalAndMergePartialHypos();

public:
  SearchNormalBatch(Manager& manager, const InputType &source, const TranslationOptionCollection &transOptColl);
  ~SearchNormalBatch();

  void ProcessSentence();

};

}

#endif
