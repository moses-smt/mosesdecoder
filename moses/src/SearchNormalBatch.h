#ifndef moses_SearchNormalBatch_h
#define moses_SearchNormalBatch_h

#include "SearchNormal.h"

namespace Moses
{

class Manager;
class InputType;
class TranslationOptionCollection;

class SearchNormalBatch: public SearchNormal
{
protected:

  // Added for asynclm decoding.
  std::vector<const StatelessFeatureFunction*> m_stateless_ffs;
  std::map<int, LanguageModel*> m_dlm_ffs;
  std::map<int, StatefulFeatureFunction*> m_stateful_ffs;  
  std::vector<Hypothesis*> m_partial_hypos;
  int m_batch_size;
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
