#include "Manager.h"
#include "Timer.h"
#include "SearchNormal.h"
#include "SentenceStats.h"
#include "FF/NeuralScoreFeature.h"

#include <boost/foreach.hpp>

using namespace std;

namespace Moses
{
  
void ExpanderNormal::operator()(const Hypothesis &hypothesis, size_t startPos, size_t endPos) {
  m_search->ExpandAllHypotheses(hypothesis, startPos, endPos);
}  

void CollectorNormal::operator()(const Hypothesis &hypothesis, size_t startPos, size_t endPos) {
  const TranslationOptionList* tol
    = m_search->m_transOptColl.GetTranslationOptionList(startPos, endPos);
  if (!tol) return;
  if(m_options.count(hypothesis.GetId()) == 0)
      m_hypotheses.push_back(&hypothesis);
  m_options[hypothesis.GetId()].push_back(tol);
}

/**
 * Organizing main function
 *
 * /param source input sentence
 * /param transOptColl collection of translation options to be used for this sentence
 */
SearchNormal::
SearchNormal(Manager& manager, const InputType &source,
             const TranslationOptionCollection &transOptColl)
  : Search(manager)
  , m_source(source)
  , m_hypoStackColl(source.GetSize() + 1)
  , m_transOptColl(transOptColl)
{
  VERBOSE(1, "Translating: " << m_source << endl);

  // m_beam_width = manager.options().search.beam_width;
  // m_stack_size = manager.options().search.stack_size;
  // m_stack_diversity = manager.options().search.stack_diversity;
  // m_timeout = manager.options().search.timeout;
  // m_max_distortion = manager.options().reordering.max_distortion;

  // only if constraint decoding (having to match a specified output)
  // long sentenceID = source.GetTranslationId();

  // initialize the stacks: create data structure and set limits
  std::vector < HypothesisStackNormal >::iterator iterStack;
  for (size_t ind = 0 ; ind < m_hypoStackColl.size() ; ++ind) {
    HypothesisStackNormal *sourceHypoColl = new HypothesisStackNormal(m_manager);
    sourceHypoColl->SetMaxHypoStackSize(this->m_options.search.stack_size,
                                        this->m_options.search.stack_diversity);
    sourceHypoColl->SetBeamWidth(this->m_options.search.beam_width);
    m_hypoStackColl[ind] = sourceHypoColl;
  }
}

SearchNormal::~SearchNormal()
{
  RemoveAllInColl(m_hypoStackColl);
}

void SearchNormal::CacheForNeural(Collector& collector) {
  const std::vector<const StatefulFeatureFunction*> &ffs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  const StaticData &staticData = StaticData::Instance();
  for (size_t i = 0; i < ffs.size(); ++i) {
    const NeuralScoreFeature* nsf = dynamic_cast<const NeuralScoreFeature*>(ffs[i]);
    if (nsf && !staticData.IsFeatureFunctionIgnored(*ffs[i]))
      const_cast<NeuralScoreFeature*>(nsf)->ProcessStack(collector, i);
  }
}

void SearchNormal::ProcessStackForNeuro(HypothesisStackNormal*& stack) {
  HypothesisStackNormal::iterator h;
  std::vector<Hypothesis*> temp;
  for (h = stack->begin(); h != stack->end(); ++h) {
    temp.push_back(*h);
    stack->Detach(h);
  }
  delete stack;
  
  stack = new HypothesisStackNormal(m_manager);
  stack->SetMaxHypoStackSize(this->m_options.search.stack_size,
                             this->m_options.search.stack_diversity);
  stack->SetBeamWidth(this->m_options.search.beam_width);
  
  const std::vector<const StatefulFeatureFunction*> &ffs = StatefulFeatureFunction::GetStatefulFeatureFunctions();
  const StaticData &staticData = StaticData::Instance();
  for (size_t i = 0; i < ffs.size(); ++i) {
    const NeuralScoreFeature* nsf = dynamic_cast<const NeuralScoreFeature*>(ffs[i]);
    if (nsf && !staticData.IsFeatureFunctionIgnored(*ffs[i]))
      const_cast<NeuralScoreFeature*>(nsf)->RescoreStack(temp, i);
  }
  
  for(int i = 0; i < temp.size(); i++)
    stack->AddPrune(temp[i]);
}

bool
SearchNormal::
ProcessOneStack(HypothesisStack* hstack, FunctorNormal* functor)
{
  if (this->out_of_time()) return false;
  SentenceStats &stats = m_manager.GetSentenceStats();
  HypothesisStackNormal* sourceHypoColl
  = static_cast<HypothesisStackNormal*>(hstack);

  // the stack is pruned before processing (lazy pruning):
  VERBOSE(3,"processing hypothesis from next stack");
  IFVERBOSE(2) stats.StartTimeStack();
  sourceHypoColl->PruneToSize(m_options.search.stack_size);
  VERBOSE(3,std::endl);
  sourceHypoColl->CleanupArcList();
  IFVERBOSE(2)  stats.StopTimeStack();

  ProcessStackForNeuro(sourceHypoColl);
  
  // go through each hypothesis on the stack and try to expand it
  // BOOST_FOREACH(Hypothesis* h, sourceHypoColl)
  HypothesisStackNormal::const_iterator h;
  for (h = sourceHypoColl->begin(); h != sourceHypoColl->end(); ++h)
    ProcessOneHypothesis(**h, functor);
  return true;
}


/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void SearchNormal::Decode()
{
  // SentenceStats &stats = m_manager.GetSentenceStats();

  // initial seed hypothesis: nothing translated, no words produced
  Hypothesis *hypo = Hypothesis::Create(m_manager, m_source, m_initialTransOpt);
  m_hypoStackColl[0]->AddPrune(hypo);

  // go through each stack
  BOOST_FOREACH(HypothesisStack* hstack, m_hypoStackColl) {
    CollectorNormal collector(this);
    if (!ProcessOneStack(hstack, &collector)) return;
    CacheForNeural(collector);

    ExpanderNormal expander(this);
    if (!ProcessOneStack(hstack, &expander)) return;
    IFVERBOSE(2) OutputHypoStackSize();
    actual_hypoStack = static_cast<HypothesisStackNormal*>(hstack);
  }
}


/** Find all translation options to expand one hypothesis, trigger expansion
 * this is mostly a check for overlap with already covered words, and for
 * violation of reordering limits.
 * \param hypothesis hypothesis to be expanded upon
 */
void
SearchNormal::
ProcessOneHypothesis(const Hypothesis &hypothesis, FunctorNormal* functor)
{
  // since we check for reordering limits, its good to have that limit handy
  // int maxDistortion  = StaticData::Instance().GetMaxDistortion();
  bool isWordLattice = m_source.GetType() == WordLatticeInput;

  const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
  const size_t hypoFirstGapPos = hypoBitmap.GetFirstGapPos();
  size_t const sourceSize = m_source.GetSize();

  ReorderingConstraint const&
  ReoConstraint = m_source.GetReorderingConstraint();

  // no limit of reordering: only check for overlap
  if (m_options.reordering.max_distortion < 0) {

    for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos) {
      TranslationOptionList const* tol;
      size_t endPos = startPos;
      for (tol = m_transOptColl.GetTranslationOptionList(startPos, endPos);
           tol && endPos < sourceSize;
           tol = m_transOptColl.GetTranslationOptionList(startPos, ++endPos)) {
        if (tol->size() == 0
            || hypoBitmap.Overlap(WordsRange(startPos, endPos))
            || !ReoConstraint.Check(hypoBitmap, startPos, endPos)) {
          continue;
        }

        //TODO: does this method include incompatible WordLattice hypotheses?
        (*functor)(hypothesis, startPos, endPos);
      }
    }
    return; // done with special case (no reordering limit)
  }

  // There are reordering limits. Make sure they are not violated.

  WordsRange prevRange = hypothesis.GetCurrSourceWordsRange();
  for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos) {

    // don't bother expanding phrases if the first position is already taken
    if(hypoBitmap.GetValue(startPos)) continue;

    size_t maxSize = sourceSize - startPos;
    size_t maxSizePhrase = m_options.search.max_phrase_length;
    maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;
    size_t closestLeft = hypoBitmap.GetEdgeToTheLeftOf(startPos);

    if (isWordLattice) {
      // first question: is there a path from the closest translated word to the left
      // of the hypothesized extension to the start of the hypothesized extension?
      // long version:
      // - is there anything to our left?
      // - is it farther left than where we're starting anyway?
      // - can we get to it?

      // closestLeft is exclusive: a value of 3 means 2 is covered, our
      // arc is currently ENDING at 3 and can start at 3 implicitly
      if (closestLeft != 0 && closestLeft != startPos
          && !m_source.CanIGetFromAToB(closestLeft, startPos))
        continue;

      if (prevRange.GetStartPos() != NOT_FOUND &&
          prevRange.GetStartPos() > startPos &&
          !m_source.CanIGetFromAToB(startPos, prevRange.GetStartPos()))
        continue;
    }

    WordsRange currentStartRange(startPos, startPos);
    if(m_source.ComputeDistortionDistance(prevRange, currentStartRange)
        > m_options.reordering.max_distortion)
      continue;

    TranslationOptionList const* tol;
    size_t endPos = startPos;
    for (tol = m_transOptColl.GetTranslationOptionList(startPos, endPos);
         tol && endPos < sourceSize;
         tol = m_transOptColl.GetTranslationOptionList(startPos, ++endPos)) {
      WordsRange extRange(startPos, endPos);
      if (tol->size() == 0
          || hypoBitmap.Overlap(extRange)
          || !ReoConstraint.Check(hypoBitmap, startPos, endPos)
          || (isWordLattice && !m_source.IsCoveragePossible(extRange))) {
        continue;
      }

      // ask second question here: we already know we can get to our
      // starting point from the closest thing to the left. We now ask the
      // follow up: can we get from our end to the closest thing on the
      // right?
      //
      // long version: is anything to our right? is it farther
      // right than our (inclusive) end? can our end reach it?
      bool isLeftMostEdge = (hypoFirstGapPos == startPos);

      size_t closestRight = hypoBitmap.GetEdgeToTheRightOf(endPos);
      if (isWordLattice) {
        if (closestRight != endPos
            && ((closestRight + 1) < sourceSize)
            && !m_source.CanIGetFromAToB(endPos + 1, closestRight + 1)) {
          continue;
        }
      }

      if (isLeftMostEdge) {
        // any length extension is okay if starting at left-most edge
        (*functor)(hypothesis, startPos, endPos);
      } else { // starting somewhere other than left-most edge, use caution
        // the basic idea is this: we would like to translate a phrase
        // starting from a position further right than the left-most
        // open gap. The distortion penalty for the following phrase
        // will be computed relative to the ending position of the
        // current extension, so we ask now what its maximum value will
        // be (which will always be the value of the hypothesis starting
        // at the left-most edge).  If this value is less than the
        // distortion limit, we don't allow this extension to be made.
        WordsRange bestNextExtension(hypoFirstGapPos, hypoFirstGapPos);

        if (m_source.ComputeDistortionDistance(extRange, bestNextExtension)
            > m_options.reordering.max_distortion) continue;

        // everything is fine, we're good to go
        (*functor)(hypothesis, startPos, endPos);
      }
    }
  }
}


/**
 * Expand a hypothesis given a list of translation options
 * \param hypothesis hypothesis to be expanded upon
 * \param startPos first word position of span covered
 * \param endPos last word position of span covered
 */

void
SearchNormal::
ExpandAllHypotheses(const Hypothesis &hypothesis, size_t startPos, size_t endPos)
{
  // early discarding: check if hypothesis is too bad to build
  // this idea is explained in (Moore&Quirk, MT Summit 2007)
  float expectedScore = 0.0f;
  if (m_options.search.UseEarlyDiscarding()) {
    // expected score is based on score of current hypothesis
    expectedScore = hypothesis.GetScore();

    // add new future score estimate
    expectedScore +=
      m_transOptColl.GetFutureScore()
      .CalcFutureScore(hypothesis.GetWordsBitmap(), startPos, endPos);
  }

  // loop through all translation options
  const TranslationOptionList* tol
  = m_transOptColl.GetTranslationOptionList(startPos, endPos);
  if (!tol) return;
  TranslationOptionList::const_iterator iter;
  for (iter = tol->begin() ; iter != tol->end() ; ++iter) {
    ExpandHypothesis(hypothesis, **iter, expectedScore);
  }
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
void SearchNormal::ExpandHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt, float expectedScore)
{
  const StaticData &staticData = StaticData::Instance();
  SentenceStats &stats = m_manager.GetSentenceStats();

  Hypothesis *newHypo;
  if (! m_options.search.UseEarlyDiscarding()) {
    // simple build, no questions asked
    IFVERBOSE(2) {
      stats.StartTimeBuildHyp();
    }
    newHypo = hypothesis.CreateNext(transOpt);
    IFVERBOSE(2) {
      stats.StopTimeBuildHyp();
    }
    if (newHypo==NULL) return;
    newHypo->EvaluateWhenApplied(m_transOptColl.GetFutureScore());
  } else
    // early discarding: check if hypothesis is too bad to build
  {
    // worst possible score may have changed -> recompute
    size_t wordsTranslated = hypothesis.GetWordsBitmap().GetNumWordsCovered() + transOpt.GetSize();
    float allowedScore = m_hypoStackColl[wordsTranslated]->GetWorstScore();
    if (m_options.search.stack_diversity) {
      WordsBitmapID id = hypothesis.GetWordsBitmap().GetIDPlus(transOpt.GetStartPos(), transOpt.GetEndPos());
      float allowedScoreForBitmap = m_hypoStackColl[wordsTranslated]->GetWorstScoreForBitmap( id );
      allowedScore = std::min( allowedScore, allowedScoreForBitmap );
    }
    allowedScore += staticData.GetEarlyDiscardingThreshold();

    // add expected score of translation option
    expectedScore += transOpt.GetFutureScore();

    // check if transOpt score push it already below limit
    if (expectedScore < allowedScore) {
      IFVERBOSE(2) {
        stats.AddNotBuilt();
      }
      return;
    }

    // build the hypothesis without scoring
    IFVERBOSE(2) {
      stats.StartTimeBuildHyp();
    }
    newHypo = hypothesis.CreateNext(transOpt);
    if (newHypo==NULL) return;
    IFVERBOSE(2) {
      stats.StopTimeBuildHyp();
    }

    // ... and check if that is below the limit
    if (expectedScore < allowedScore) {
      IFVERBOSE(2) {
        stats.AddEarlyDiscarded();
      }
      FREEHYPO( newHypo );
      return;
    }

  }

  // logging for the curious
  IFVERBOSE(3) {
    newHypo->PrintHypothesis();
  }

  // add to hypothesis stack
  size_t wordsTranslated = newHypo->GetWordsBitmap().GetNumWordsCovered();
  IFVERBOSE(2) {
    stats.StartTimeStack();
  }
  m_hypoStackColl[wordsTranslated]->AddPrune(newHypo);
  IFVERBOSE(2) {
    stats.StopTimeStack();
  }
}

const std::vector < HypothesisStack* >& SearchNormal::GetHypothesisStacks() const
{
  return m_hypoStackColl;
}

/**
 * Find best hypothesis on the last stack.
 * This is the end point of the best translation, which can be traced back from here
 */
const Hypothesis *SearchNormal::GetBestHypothesis() const
{
  if (interrupted_flag == 0) {
    const HypothesisStackNormal &hypoColl = *static_cast<HypothesisStackNormal*>(m_hypoStackColl.back());
    return hypoColl.GetBestHypothesis();
  } else {
    const HypothesisStackNormal &hypoColl = *actual_hypoStack;
    return hypoColl.GetBestHypothesis();
  }
}

/**
 * Logging of hypothesis stack sizes
 */
void SearchNormal::OutputHypoStackSize()
{
  std::vector < HypothesisStack* >::const_iterator iterStack = m_hypoStackColl.begin();
  TRACE_ERR( "Stack sizes: " << (int)(*iterStack)->size());
  for (++iterStack; iterStack != m_hypoStackColl.end() ; ++iterStack) {
    TRACE_ERR( ", " << (int)(*iterStack)->size());
  }
  TRACE_ERR( endl);
}

void SearchNormal::OutputHypoStack()
{
  // all stacks
  int i = 0;
  vector < HypothesisStack* >::iterator iterStack;
  for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack) {
    HypothesisStackNormal &hypoColl = *static_cast<HypothesisStackNormal*>(*iterStack);
    TRACE_ERR( "Stack " << i++ << ": " << endl << hypoColl << endl);
  }
}

}
