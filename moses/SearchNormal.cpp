#include "Manager.h"
#include "Timer.h"
#include "SearchNormal.h"
#include "SentenceStats.h"

#include <boost/foreach.hpp>

using namespace std;

namespace Moses
{
/**
 * Organizing main function
 *
 * /param source input sentence
 * /param transOptColl collection of translation options to be used for this sentence
 */
SearchNormal::
SearchNormal(Manager& manager, const TranslationOptionCollection &transOptColl)
  : Search(manager)
  , m_hypoStackColl(manager.GetSource().GetSize() + 1)
  , m_transOptColl(transOptColl)
{
  VERBOSE(1, "Translating: " << m_source << endl);

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


bool
SearchNormal::
ProcessOneStack(HypothesisStack* hstack)
{
  if (this->out_of_time()) return false;
  SentenceStats &stats = m_manager.GetSentenceStats();
  HypothesisStackNormal &sourceHypoColl
  = *static_cast<HypothesisStackNormal*>(hstack);

  // the stack is pruned before processing (lazy pruning):
  VERBOSE(3,"processing hypothesis from next stack");
  IFVERBOSE(2) stats.StartTimeStack();
  sourceHypoColl.PruneToSize(m_options.search.stack_size);
  VERBOSE(3,std::endl);
  sourceHypoColl.CleanupArcList();
  IFVERBOSE(2)  stats.StopTimeStack();

  // go through each hypothesis on the stack and try to expand it
  // BOOST_FOREACH(Hypothesis* h, sourceHypoColl)
  HypothesisStackNormal::const_iterator h;
  for (h = sourceHypoColl.begin(); h != sourceHypoColl.end(); ++h)
    ProcessOneHypothesis(**h);
  return true;
}


/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void SearchNormal::Decode()
{
  // initial seed hypothesis: nothing translated, no words produced
  const Bitmap &initBitmap = m_bitmaps.GetInitialBitmap();
  Hypothesis *hypo = new Hypothesis(m_manager, m_source, m_initialTransOpt, initBitmap, m_manager.GetNextHypoId());

  m_hypoStackColl[0]->AddPrune(hypo);

  // go through each stack
  BOOST_FOREACH(HypothesisStack* hstack, m_hypoStackColl) {
    if (!ProcessOneStack(hstack)) return;
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
ProcessOneHypothesis(const Hypothesis &hypothesis)
{
  // since we check for reordering limits, its good to have that limit handy
  bool isWordLattice = m_source.GetType() == WordLatticeInput;

  const Bitmap &hypoBitmap = hypothesis.GetWordsBitmap();
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
            || hypoBitmap.Overlap(Range(startPos, endPos))
            || !ReoConstraint.Check(hypoBitmap, startPos, endPos)) {
          continue;
        }

        //TODO: does this method include incompatible WordLattice hypotheses?
        ExpandAllHypotheses(hypothesis, startPos, endPos);
      }
    }
    return; // done with special case (no reordering limit)
  }

  // There are reordering limits. Make sure they are not violated.

  Range prevRange = hypothesis.GetCurrSourceWordsRange();
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

    Range currentStartRange(startPos, startPos);
    if(m_source.ComputeDistortionDistance(prevRange, currentStartRange)
        > m_options.reordering.max_distortion)
      continue;

    TranslationOptionList const* tol;
    size_t endPos = startPos;
    for (tol = m_transOptColl.GetTranslationOptionList(startPos, endPos);
         tol && endPos < sourceSize;
         tol = m_transOptColl.GetTranslationOptionList(startPos, ++endPos)) {
      Range extRange(startPos, endPos);
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
        ExpandAllHypotheses(hypothesis, startPos, endPos);
      } else { // starting somewhere other than left-most edge, use caution
        // the basic idea is this: we would like to translate a phrase
        // starting from a position further right than the left-most
        // open gap. The distortion penalty for the following phrase
        // will be computed relative to the ending position of the
        // current extension, so we ask now what its maximum value will
        // be (which will always be the value of the hypothesis starting
        // at the left-most edge).  If this value is less than the
        // distortion limit, we don't allow this extension to be made.
        Range bestNextExtension(hypoFirstGapPos, hypoFirstGapPos);

        if (m_source.ComputeDistortionDistance(extRange, bestNextExtension)
            > m_options.reordering.max_distortion) continue;

        // everything is fine, we're good to go
        ExpandAllHypotheses(hypothesis, startPos, endPos);
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

  const Bitmap &sourceCompleted = hypothesis.GetWordsBitmap();
  float estimatedScore = m_transOptColl.GetEstimatedScores().CalcEstimatedScore( sourceCompleted, startPos, endPos );

  const Range &hypoRange = hypothesis.GetCurrSourceWordsRange();
  //cerr << "DOING " << sourceCompleted << " [" << hypoRange.GetStartPos() << " " << hypoRange.GetEndPos() << "]"
  //		  " [" << startPos << " " << endPos << "]" << endl;

  if (m_options.search.UseEarlyDiscarding()) {
    // expected score is based on score of current hypothesis
    expectedScore = hypothesis.GetScore();

    // add new future score estimate
    expectedScore += estimatedScore;
  }

  // loop through all translation options
  const TranslationOptionList* tol
  = m_transOptColl.GetTranslationOptionList(startPos, endPos);
  if (!tol || tol->size() == 0) return;

  // Create new bitmap
  const TranslationOption &transOpt = **tol->begin();
  const Range &nextRange = transOpt.GetSourceWordsRange();
  const Bitmap &nextBitmap = m_bitmaps.GetBitmap(sourceCompleted, nextRange);

  TranslationOptionList::const_iterator iter;
  for (iter = tol->begin() ; iter != tol->end() ; ++iter) {
    const TranslationOption &transOpt = **iter;
    ExpandHypothesis(hypothesis, transOpt, expectedScore, estimatedScore, nextBitmap);
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
void SearchNormal::ExpandHypothesis(const Hypothesis &hypothesis,
                                    const TranslationOption &transOpt,
                                    float expectedScore,
                                    float estimatedScore,
                                    const Bitmap &bitmap)
{
  SentenceStats &stats = m_manager.GetSentenceStats();

  Hypothesis *newHypo;
  if (! m_options.search.UseEarlyDiscarding()) {
    // simple build, no questions asked
    IFVERBOSE(2) {
      stats.StartTimeBuildHyp();
    }
    newHypo = new Hypothesis(hypothesis, transOpt, bitmap, m_manager.GetNextHypoId());
    IFVERBOSE(2) {
      stats.StopTimeBuildHyp();
    }
    if (newHypo==NULL) return;

    IFVERBOSE(2) {
      m_manager.GetSentenceStats().StartTimeOtherScore();
    }
    newHypo->EvaluateWhenApplied(estimatedScore);
    IFVERBOSE(2) {
      m_manager.GetSentenceStats().StopTimeOtherScore();

      // TODO: these have been meaningless for a while.
      // At least since commit 67fb5c
      // should now be measured in SearchNormal.cpp:254 instead, around CalcFutureScore2()
      // CalcFutureScore2() also called in BackwardsEdge::Initialize().
      //
      // however, CalcFutureScore2() should be quick
      // since it uses dynamic programming results in SquareMatrix
      m_manager.GetSentenceStats().StartTimeEstimateScore();
      m_manager.GetSentenceStats().StopTimeEstimateScore();
    }
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
    allowedScore += m_options.search.early_discarding_threshold;

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
    newHypo = new Hypothesis(hypothesis, transOpt, bitmap, m_manager.GetNextHypoId());
    if (newHypo==NULL) return;
    IFVERBOSE(2) {
      stats.StopTimeBuildHyp();
    }

    // ... and check if that is below the limit
    if (expectedScore < allowedScore) {
      IFVERBOSE(2) {
        stats.AddEarlyDiscarded();
      }
      delete newHypo;
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
