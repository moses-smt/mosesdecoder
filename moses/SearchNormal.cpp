#include "Manager.h"
#include "Timer.h"
#include "SearchNormal.h"
#include "SentenceStats.h"

using namespace std;

namespace Moses
{
/**
 * Organizing main function
 *
 * /param source input sentence
 * /param transOptColl collection of translation options to be used for this sentence
 */
SearchNormal::SearchNormal(Manager& manager, const InputType &source, const TranslationOptionCollection &transOptColl)
  :Search(manager)
  ,m_source(source)
  ,m_hypoStackColl(source.GetSize() + 1)
  ,m_start(clock())
  ,interrupted_flag(0)
  ,m_transOptColl(transOptColl)
{
  VERBOSE(1, "Translating: " << m_source << endl);
  const StaticData &staticData = StaticData::Instance();

  // only if constraint decoding (having to match a specified output)
  // long sentenceID = source.GetTranslationId();

  // initialize the stacks: create data structure and set limits
  std::vector < HypothesisStackNormal >::iterator iterStack;
  for (size_t ind = 0 ; ind < m_hypoStackColl.size() ; ++ind) {
    HypothesisStackNormal *sourceHypoColl = new HypothesisStackNormal(m_manager);
    sourceHypoColl->SetMaxHypoStackSize(staticData.GetMaxHypoStackSize(),
                                        staticData.GetMinHypoStackDiversity());
    sourceHypoColl->SetBeamWidth(staticData.GetBeamWidth());

    m_hypoStackColl[ind] = sourceHypoColl;
  }
}

SearchNormal::~SearchNormal()
{
  RemoveAllInColl(m_hypoStackColl);
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void SearchNormal::ProcessSentence()
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
    // some logging
    IFVERBOSE(2) {
      OutputHypoStackSize();
    }

    // this stack is fully expanded;
    actual_hypoStack = &sourceHypoColl;
  }

  // some more logging
  IFVERBOSE(2) {
    m_manager.GetSentenceStats().SetTimeTotal( clock()-m_start );
  }
  VERBOSE(2, m_manager.GetSentenceStats());
}


/** Find all translation options to expand one hypothesis, trigger expansion
 * this is mostly a check for overlap with already covered words, and for
 * violation of reordering limits.
 * \param hypothesis hypothesis to be expanded upon
 */
void SearchNormal::ProcessOneHypothesis(const Hypothesis &hypothesis)
{
  // since we check for reordering limits, its good to have that limit handy
  int maxDistortion = StaticData::Instance().GetMaxDistortion();
  bool isWordLattice = StaticData::Instance().GetInputType() == WordLatticeInput;

  // no limit of reordering: only check for overlap
  if (maxDistortion < 0) {
    const WordsBitmap hypoBitmap	= hypothesis.GetWordsBitmap();
    const size_t hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
                                    , sourceSize			= m_source.GetSize();

    for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos) {
      size_t maxSize = sourceSize - startPos;
      size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
      maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;

      for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos) {
        // basic checks
        // there have to be translation options
        if (m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos)).size() == 0 ||
            // no overlap with existing words
            hypoBitmap.Overlap(WordsRange(startPos, endPos)) ||
            // specified reordering constraints (set with -monotone-at-punctuation or xml)
            !m_source.GetReorderingConstraint().Check( hypoBitmap, startPos, endPos ) ) {
          continue;
        }

        //TODO: does this method include incompatible WordLattice hypotheses?
        ExpandAllHypotheses(hypothesis, startPos, endPos);
      }
    }

    return; // done with special case (no reordering limit)
  }

  // if there are reordering limits, make sure it is not violated
  // the coverage bitmap is handy here (and the position of the first gap)
  const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
  const size_t	hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
                                  , sourceSize			= m_source.GetSize();

  // MAIN LOOP. go through each possible range
  for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos) {
    // don't bother expanding phrases if the first position is already taken
    if(hypoBitmap.GetValue(startPos))
      continue;

    WordsRange prevRange = hypothesis.GetCurrSourceWordsRange();

    size_t maxSize = sourceSize - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
    maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;
    size_t closestLeft = hypoBitmap.GetEdgeToTheLeftOf(startPos);
    if (isWordLattice) {
      // first question: is there a path from the closest translated word to the left
      // of the hypothesized extension to the start of the hypothesized extension?
      // long version: is there anything to our left? is it farther left than where we're starting anyway? can we get to it?
      // closestLeft is exclusive: a value of 3 means 2 is covered, our arc is currently ENDING at 3 and can start at 3 implicitly
      if (closestLeft != 0 && closestLeft != startPos && !m_source.CanIGetFromAToB(closestLeft, startPos)) {
        continue;
      }
      if (prevRange.GetStartPos() != NOT_FOUND &&
          prevRange.GetStartPos() > startPos && !m_source.CanIGetFromAToB(startPos, prevRange.GetStartPos())) {
        continue;
      }
    }

    WordsRange currentStartRange(startPos, startPos);
    if(m_source.ComputeDistortionDistance(prevRange, currentStartRange) > maxDistortion)
      continue;

    for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos) {
      // basic checks
      WordsRange extRange(startPos, endPos);
      // there have to be translation options
      if (m_transOptColl.GetTranslationOptionList(extRange).size() == 0 ||
          // no overlap with existing words
          hypoBitmap.Overlap(extRange) ||
          // specified reordering constraints (set with -monotone-at-punctuation or xml)
          !m_source.GetReorderingConstraint().Check( hypoBitmap, startPos, endPos ) || //
          // connection in input word lattice
          (isWordLattice && !m_source.IsCoveragePossible(extRange))) {
        continue;
      }

      // ask second question here:
      // we already know we can get to our starting point from the closest thing to the left. We now ask the follow up:
      // can we get from our end to the closest thing on the right?
      // long version: is anything to our right? is it farther right than our (inclusive) end? can our end reach it?
      bool leftMostEdge = (hypoFirstGapPos == startPos);

      // closest right definition:
      size_t closestRight = hypoBitmap.GetEdgeToTheRightOf(endPos);
      if (isWordLattice) {
        //if (!leftMostEdge && closestRight != endPos && closestRight != sourceSize && !m_source.CanIGetFromAToB(endPos, closestRight + 1)) {
        if (closestRight != endPos && ((closestRight + 1) < sourceSize) && !m_source.CanIGetFromAToB(endPos + 1, closestRight + 1)) {
          continue;
        }
      }

      // any length extension is okay if starting at left-most edge
      if (leftMostEdge) {
        ExpandAllHypotheses(hypothesis, startPos, endPos);
      }
      // starting somewhere other than left-most edge, use caution
      else {
        // the basic idea is this: we would like to translate a phrase starting
        // from a position further right than the left-most open gap. The
        // distortion penalty for the following phrase will be computed relative
        // to the ending position of the current extension, so we ask now what
        // its maximum value will be (which will always be the value of the
        // hypothesis starting at the left-most edge).  If this value is less than
        // the distortion limit, we don't allow this extension to be made.
        WordsRange bestNextExtension(hypoFirstGapPos, hypoFirstGapPos);
        int required_distortion =
          m_source.ComputeDistortionDistance(extRange, bestNextExtension);

        if (required_distortion > maxDistortion) {
          continue;
        }

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

void SearchNormal::ExpandAllHypotheses(const Hypothesis &hypothesis, size_t startPos, size_t endPos)
{
  // early discarding: check if hypothesis is too bad to build
  // this idea is explained in (Moore&Quirk, MT Summit 2007)
  float expectedScore = 0.0f;
  if (StaticData::Instance().UseEarlyDiscarding()) {
    // expected score is based on score of current hypothesis
    expectedScore = hypothesis.GetScore();

    // add new future score estimate
    expectedScore += m_transOptColl.GetFutureScore().CalcFutureScore( hypothesis.GetWordsBitmap(), startPos, endPos );
  }

  // loop through all translation options
  const TranslationOptionList &transOptList = m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos));
  TranslationOptionList::const_iterator iter;
  for (iter = transOptList.begin() ; iter != transOptList.end() ; ++iter) {
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
    newHypo->Evaluate(m_transOptColl.GetFutureScore());
  } else
    // early discarding: check if hypothesis is too bad to build
  {
    // worst possible score may have changed -> recompute
    size_t wordsTranslated = hypothesis.GetWordsBitmap().GetNumWordsCovered() + transOpt.GetSize();
    float allowedScore = m_hypoStackColl[wordsTranslated]->GetWorstScore();
    if (staticData.GetMinHypoStackDiversity()) {
      WordsBitmapID id = hypothesis.GetWordsBitmap().GetIDPlus(transOpt.GetStartPos(), transOpt.GetEndPos());
      float allowedScoreForBitmap = m_hypoStackColl[wordsTranslated]->GetWorstScoreForBitmap( id );
      allowedScore = std::min( allowedScore, allowedScoreForBitmap );
    }
    allowedScore += staticData.GetEarlyDiscardingThreshold();

    // add expected score of translation option
    expectedScore += transOpt.GetFutureScore();
    // TRACE_ERR("EXPECTED diff: " << (newHypo->GetTotalScore()-expectedScore) << " (pre " << (newHypo->GetTotalScore()-expectedScorePre) << ") " << hypothesis.GetTargetPhrase() << " ... " << transOpt.GetTargetPhrase() << " [" << expectedScorePre << "," << expectedScore << "," << newHypo->GetTotalScore() << "]" << endl);

    // check if transOpt score push it already below limit
    if (expectedScore < allowedScore) {
      IFVERBOSE(2) {
        stats.AddNotBuilt();
      }
      return;
    }

    // build the hypothesis without scoring
    IFVERBOSE(2) {
      t = clock();
    }
    newHypo = hypothesis.CreateNext(transOpt);
    if (newHypo==NULL) return;
    IFVERBOSE(2) {
      stats.AddTimeBuildHyp( clock()-t );
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
    t = clock();
  }
  m_hypoStackColl[wordsTranslated]->AddPrune(newHypo);
  IFVERBOSE(2) {
    stats.AddTimeStack( clock()-t );
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

}
