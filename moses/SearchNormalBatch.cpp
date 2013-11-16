#include "SearchNormalBatch.h"
#include "LM/Base.h"
#include "Manager.h"
#include "Hypothesis.h"

//#include <google/profiler.h>

using namespace std;

namespace Moses
{
SearchNormalBatch::SearchNormalBatch(Manager& manager, const InputType &source, const TranslationOptionCollection &transOptColl)
  :SearchNormal(manager, source, transOptColl)
  ,m_batch_size(10000)
{
  m_max_stack_size = StaticData::Instance().GetMaxHypoStackSize();

  // Split the feature functions into sets of stateless, stateful
  // distributed lm, and stateful non-distributed.
  const vector<const StatefulFeatureFunction*>& ffs =
    StatefulFeatureFunction::GetStatefulFeatureFunctions();
  for (unsigned i = 0; i < ffs.size(); ++i) {
    if (ffs[i]->GetScoreProducerDescription() == "DLM_5gram") { // TODO WFT
      m_dlm_ffs[i] = const_cast<LanguageModel*>(static_cast<const LanguageModel* const>(ffs[i]));
      m_dlm_ffs[i]->SetFFStateIdx(i);
    } else {
      m_stateful_ffs[i] = const_cast<StatefulFeatureFunction*>(ffs[i]);
    }
  }
  m_stateless_ffs = StatelessFeatureFunction::GetStatelessFeatureFunctions();

}

SearchNormalBatch::~SearchNormalBatch()
{
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void SearchNormalBatch::ProcessSentence()
{
  const StaticData &staticData = StaticData::Instance();
  SentenceStats &stats = m_manager.GetSentenceStats();
  clock_t t=0; // used to track time for steps

  // initial seed hypothesis: nothing translated, no words produced
  Hypothesis *hypo = Hypothesis::Create(m_manager,m_source, m_initialTransOpt);
  m_hypoStackColl[0]->AddPrune(hypo);

  // go through each stack
  std::vector < HypothesisStack* >::iterator iterStack;
  for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack) {
    // check if decoding ran out of time
    double _elapsed_time = GetUserTime();
    if (_elapsed_time > staticData.GetTimeoutThreshold()) {
      VERBOSE(1,"Decoding is out of time (" << _elapsed_time << "," << staticData.GetTimeoutThreshold() << ")" << std::endl);
      interrupted_flag = 1;
      return;
    }
    HypothesisStackNormal &sourceHypoColl = *static_cast<HypothesisStackNormal*>(*iterStack);

    // the stack is pruned before processing (lazy pruning):
    VERBOSE(3,"processing hypothesis from next stack");
    IFVERBOSE(2) {
      t = clock();
    }
    sourceHypoColl.PruneToSize(staticData.GetMaxHypoStackSize());
    VERBOSE(3,std::endl);
    sourceHypoColl.CleanupArcList();
    IFVERBOSE(2) {
      stats.AddTimeStack( clock()-t );
    }

    // go through each hypothesis on the stack and try to expand it
    HypothesisStackNormal::const_iterator iterHypo;
    for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo) {
      Hypothesis &hypothesis = **iterHypo;
      ProcessOneHypothesis(hypothesis); // expand the hypothesis
    }
    EvalAndMergePartialHypos();

    // some logging
    IFVERBOSE(2) {
      OutputHypoStackSize();
    }

    // this stack is fully expanded;
    actual_hypoStack = &sourceHypoColl;
  }

  EvalAndMergePartialHypos();

  // some more logging
  IFVERBOSE(2) {
    m_manager.GetSentenceStats().SetTimeTotal( clock()-m_start );
  }
  VERBOSE(2, m_manager.GetSentenceStats());
}

/**
 * Expand one hypothesis with a translation option.
 * this involves initial creation, scoring and adding it to the proper stack
 * \param hypothesis hypothesis to be expanded upon
 * \param transOpt translation option (phrase translation)
 *        that is applied to create the new hypothesis
 * \param expectedScore base score for early discarding
 *        (base hypothesis score plus future score estimation)
 */

void
SearchNormalBatch::
ExpandHypothesis(const Hypothesis &hypothesis,
                 const TranslationOption &transOpt, float expectedScore)
{
  // Check if the number of partial hypotheses exceeds the batch size.
  if (m_partial_hypos.size() >= m_batch_size) {
    EvalAndMergePartialHypos();
  }

  const StaticData &staticData = StaticData::Instance();
  SentenceStats &stats = m_manager.GetSentenceStats();
  clock_t t=0; // used to track time for steps

  Hypothesis *newHypo;
  if (! staticData.UseEarlyDiscarding()) {
    // simple build, no questions asked
    IFVERBOSE(2) {
      t = clock();
    }
    newHypo = hypothesis.CreateNext(transOpt);
    IFVERBOSE(2) {
      stats.AddTimeBuildHyp( clock()-t );
    }
    if (newHypo==NULL) return;
    //newHypo->Evaluate(m_transOptColl.GetFutureScore());

    // Issue DLM requests for new hypothesis and put into the list of
    // partial hypotheses.
    std::map<int, LanguageModel*>::iterator dlm_iter;
    for (dlm_iter = m_dlm_ffs.begin();
         dlm_iter != m_dlm_ffs.end();
         ++dlm_iter) {
      const FFState* input_state = newHypo->GetPrevHypo() ? newHypo->GetPrevHypo()->GetFFState((*dlm_iter).first) : NULL;
      (*dlm_iter).second->IssueRequestsFor(*newHypo, input_state);
    }
    m_partial_hypos.push_back(newHypo);
  } else {
    std::cerr << "can't use early discarding with batch decoding!" << std::endl;
    abort();
  }
}

void SearchNormalBatch::EvalAndMergePartialHypos()
{
  std::vector<Hypothesis*>::iterator partial_hypo_iter;
  for (partial_hypo_iter = m_partial_hypos.begin();
       partial_hypo_iter != m_partial_hypos.end();
       ++partial_hypo_iter) {
    Hypothesis* hypo = *partial_hypo_iter;

    // Evaluate with other ffs.
    std::map<int, StatefulFeatureFunction*>::iterator sfff_iter;
    for (sfff_iter = m_stateful_ffs.begin();
         sfff_iter != m_stateful_ffs.end();
         ++sfff_iter) {
      const StatefulFeatureFunction &ff = *(sfff_iter->second);
      int state_idx = sfff_iter->first;
      hypo->EvaluateWith(ff, state_idx);
    }
    std::vector<const StatelessFeatureFunction*>::iterator slff_iter;
    for (slff_iter = m_stateless_ffs.begin();
         slff_iter != m_stateless_ffs.end();
         ++slff_iter) {
      hypo->EvaluateWith(**slff_iter);
    }
  }

  // Wait for all requests from the distributed LM to come back.
  std::map<int, LanguageModel*>::iterator dlm_iter;
  for (dlm_iter = m_dlm_ffs.begin();
       dlm_iter != m_dlm_ffs.end();
       ++dlm_iter) {
    (*dlm_iter).second->sync();
  }

  // Incorporate the DLM scores into all hypotheses and put into their
  // stacks.
  for (partial_hypo_iter = m_partial_hypos.begin();
       partial_hypo_iter != m_partial_hypos.end();
       ++partial_hypo_iter) {
    Hypothesis* hypo = *partial_hypo_iter;

    // Calculate DLM scores.
    std::map<int, LanguageModel*>::iterator dlm_iter;
    for (dlm_iter = m_dlm_ffs.begin();
         dlm_iter != m_dlm_ffs.end();
         ++dlm_iter) {
      LanguageModel &lm = *(dlm_iter->second);
      hypo->EvaluateWith(lm, (*dlm_iter).first);
    }

    // Put completed hypothesis onto its stack.
    size_t wordsTranslated = hypo->GetWordsBitmap().GetNumWordsCovered();
    m_hypoStackColl[wordsTranslated]->AddPrune(hypo);
  }
  m_partial_hypos.clear();

  std::vector < HypothesisStack* >::iterator stack_iter;
  HypothesisStackNormal* stack;
  for (stack_iter = m_hypoStackColl.begin();
       stack_iter != m_hypoStackColl.end();
       ++stack_iter) {
    stack = static_cast<HypothesisStackNormal*>(*stack_iter);
    stack->PruneToSize(m_max_stack_size);
  }
}

}
