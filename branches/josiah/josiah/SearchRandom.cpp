#include "Timer.h"
#include "SearchRandom.h"

using namespace Moses;

namespace Josiah
{
/**
 * Organizing main function
 *
 * /param source input sentence
 * /param transOptColl collection of translation options to be used for this sentence
 */
SearchRandom::SearchRandom(const InputType &source, const TranslationOptionCollection &transOptColl)
	:m_source(source)
	,m_hypoStackColl(source.GetSize() + 1)
	,m_initialTargetPhrase(Output)
	,m_transOptColl(transOptColl)
{
	VERBOSE(1, "Translating: " << m_source << endl);
	const StaticData &staticData = StaticData::Instance();

	// only if constraint decoding (having to match a specified output)
	long sentenceID = source.GetTranslationId();
	m_constraint = staticData.GetConstrainingPhrase(sentenceID);

	// initialize the stacks: create data structure and set limits
	std::vector < HypothesisStackRandom >::iterator iterStack;
	for (size_t ind = 0 ; ind < m_hypoStackColl.size() ; ++ind)
	{
		HypothesisStackRandom *sourceHypoColl = new HypothesisStackRandom();
		sourceHypoColl->SetMaxHypoStackSize(staticData.GetMaxHypoStackSize(),staticData.GetMinHypoStackDiversity());
		sourceHypoColl->SetBeamWidth(staticData.GetBeamWidth());

		m_hypoStackColl[ind] = sourceHypoColl;
	}
}

SearchRandom::~SearchRandom()
{
	RemoveAllInColl(m_hypoStackColl);
}

/**
 * Main decoder loop that translates a sentence by expanding
 * hypotheses stack by stack, until the end of the sentence.
 */
void SearchRandom::ProcessSentence()
{
	const StaticData &staticData = StaticData::Instance();
	SentenceStats &stats = staticData.GetSentenceStats();


	// initial seed hypothesis: nothing translated, no words produced
	Hypothesis *hypo = Hypothesis::Create(m_source, m_initialTargetPhrase);
	m_hypoStackColl[0]->AddPrune(hypo);

	// go through each stack
	std::vector < HypothesisStack* >::iterator iterStack;
	for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack)
	{
		
		HypothesisStackRandom &sourceHypoColl = *static_cast<HypothesisStackRandom*>(*iterStack);

		// the stack is pruned before processing (lazy pruning):
		VERBOSE(3,"processing hypothesis from next stack");
		sourceHypoColl.PruneToSize(staticData.GetMaxHypoStackSize());
		VERBOSE(3,std::endl);
		sourceHypoColl.CleanupArcList();

		// go through each hypothesis on the stack and try to expand it
		HypothesisStackRandom::const_iterator iterHypo;
		for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
		{
			Hypothesis &hypothesis = **iterHypo;
			ProcessOneHypothesis(hypothesis); // expand the hypothesis
		}
		// some logging
		IFVERBOSE(2) { OutputHypoStackSize(); }

		// this stack is fully expanded;
		actual_hypoStack = &sourceHypoColl;
	}

	// some more logging
	VERBOSE(2, staticData.GetSentenceStats());
}


/** Find all translation options to expand one hypothesis, trigger expansion
 * this is mostly a check for overlap with already covered words, and for
 * violation of reordering limits.
 * \param hypothesis hypothesis to be expanded upon
 */
void SearchRandom::ProcessOneHypothesis(const Hypothesis &hypothesis)
{
	// since we check for reordering limits, its good to have that limit handy
	int maxDistortion = StaticData::Instance().GetMaxDistortion();
	bool isWordLattice = StaticData::Instance().GetInputType() == WordLatticeInput;

	// no limit of reordering: only check for overlap
	if (maxDistortion < 0)
	{
		const WordsBitmap hypoBitmap	= hypothesis.GetWordsBitmap();
		const size_t hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
			, sourceSize			= m_source.GetSize();

		for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
		{
			size_t maxSize = sourceSize - startPos;
			size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
			maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;

			for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos)
			{
				// basic checks
				    // there have to be translation options
				if (m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos)).size() == 0 ||
				    // no overlap with existing words
				    hypoBitmap.Overlap(WordsRange(startPos, endPos)) ||
				    // specified reordering constraints (set with -monotone-at-punctuation or xml)
				    !m_source.GetReorderingConstraint().Check( hypoBitmap, startPos, endPos ) )
				{
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
	for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
	{
		int lastEnd = static_cast<int>(hypothesis.GetCurrSourceWordsRange().GetEndPos());
		if (startPos != 0 && (static_cast<int>(startPos) - lastEnd - 1) > maxDistortion)
			continue;
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
		}

		for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos)
		{
			// basic checks
			WordsRange extRange(startPos, endPos);
			    // there have to be translation options
			if (m_transOptColl.GetTranslationOptionList(extRange).size() == 0 ||
			    // no overlap with existing words
			    hypoBitmap.Overlap(extRange) ||
			    // specified reordering constraints (set with -monotone-at-punctuation or xml)
			    !m_source.GetReorderingConstraint().Check( hypoBitmap, startPos, endPos ) || //
			    // connection in input word lattice
			    (isWordLattice && !m_source.IsCoveragePossible(extRange)))
			{
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
				if (closestRight != endPos && ((closestRight + 1) < sourceSize) && !m_source.CanIGetFromAToB(endPos, closestRight + 1)) {
					continue;
				}
			}

			// any length extension is okay if starting at left-most edge
			if (leftMostEdge)
			{
				ExpandAllHypotheses(hypothesis, startPos, endPos);
			}
			// starting somewhere other than left-most edge, use caution
			else
			{
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

void SearchRandom::ExpandAllHypotheses(const Hypothesis &hypothesis, size_t startPos, size_t endPos)
{
	// early discarding: check if hypothesis is too bad to build
	// this idea is explained in (Moore&Quirk, MT Summit 2007)
	float expectedScore = 0.0f;
	/*if (StaticData::Instance().UseEarlyDiscarding())
	{
		// expected score is based on score of current hypothesis
		expectedScore = hypothesis.GetScore();

		// add new future score estimate
		expectedScore += m_transOptColl.GetFutureScore().CalcFutureScore( hypothesis.GetWordsBitmap(), startPos, endPos );
  }*/

	// loop through all translation options
	const TranslationOptionList &transOptList = m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos));
	TranslationOptionList::const_iterator iter;
	for (iter = transOptList.begin() ; iter != transOptList.end() ; ++iter)
	{
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
void SearchRandom::ExpandHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt, float expectedScore)
{
	const StaticData &staticData = StaticData::Instance();
	SentenceStats &stats = staticData.GetSentenceStats();

	Hypothesis *newHypo;
	// simple build, no questions asked
	newHypo = hypothesis.CreateNext(transOpt, m_constraint);
	if (newHypo==NULL) return;
	newHypo->CalcScore(m_transOptColl.GetFutureScore());
  //create random scores
  /*RandomNumberGenerator& random = RandomNumberGenerator::instance();
  ScoreComponentCollection scores;
  for (size_t i = 0; i < scores.size(); ++i) {
    scores[i] = random.next();
  }
  newHypo->SetScore(scores,random.next(),random.next());*/

	// logging for the curious
	IFVERBOSE(3) {
		newHypo->PrintHypothesis();
	}

	// add to hypothesis stack
	size_t wordsTranslated = newHypo->GetWordsBitmap().GetNumWordsCovered();
	m_hypoStackColl[wordsTranslated]->AddPrune(newHypo);
}

const std::vector < HypothesisStack* >& SearchRandom::GetHypothesisStacks() const
{
	return m_hypoStackColl;
}

/**
 * Find best hypothesis on the last stack.
 * This is the end point of the best translation, which can be traced back from here
 */
const Hypothesis *SearchRandom::GetBestHypothesis() const
{
		const HypothesisStackRandom &hypoColl = *actual_hypoStack;
    Hypothesis* bestHypo = const_cast<Hypothesis*>(hypoColl.GetBestHypothesis());
    //bestHypo->CalcScore(m_transOptColl.GetFutureScore());
		return bestHypo;
}

/**
 * Logging of hypothesis stack sizes
 */
void SearchRandom::OutputHypoStackSize()
{
	std::vector < HypothesisStack* >::const_iterator iterStack = m_hypoStackColl.begin();
	TRACE_ERR( "Stack sizes: " << (int)(*iterStack)->size());
	for (++iterStack; iterStack != m_hypoStackColl.end() ; ++iterStack)
	{
		TRACE_ERR( ", " << (int)(*iterStack)->size());
	}
	TRACE_ERR( endl);
}

  HypothesisStackRandom::HypothesisStackRandom()
  {
    m_nBestIsEnabled = StaticData::Instance().IsNBestEnabled();
    m_bestScore = -std::numeric_limits<float>::infinity();
    m_worstScore = -std::numeric_limits<float>::infinity();
  }

  /** remove all hypotheses from the collection */
  void HypothesisStackRandom::RemoveAll()
  {
    while (m_hypos.begin() != m_hypos.end())
    {
      Remove(m_hypos.begin());
    }
  }

  pair<HypothesisStackRandom::iterator, bool> HypothesisStackRandom::Add(Hypothesis *hypo)
  {
    std::pair<iterator, bool> ret = m_hypos.insert(hypo);
    if (ret.second) 
    { // equiv hypo doesn't exists
      VERBOSE(3,"added hyp to stack");
  
    // Update best score, if this hypothesis is new best
      if (GetTotalScore(hypo) > m_bestScore)
      {
        VERBOSE(3,", best on stack");
        m_bestScore = GetTotalScore(hypo);
      // this may also affect the worst score
        if ( m_bestScore + m_beamWidth > m_worstScore )
          m_worstScore = m_bestScore + m_beamWidth;
      }
    // update best/worst score for stack diversity 1
      if ( m_minHypoStackDiversity == 1 && 
           GetTotalScore(hypo) > GetWorstScoreForBitmap( hypo->GetWordsBitmap() ) )
      {
        SetWorstScoreForBitmap( hypo->GetWordsBitmap().GetID(), GetTotalScore(hypo) );
      }
  
      VERBOSE(3,", now size " << m_hypos.size());

    // prune only if stack is twice as big as needed (lazy pruning)
      size_t toleratedSize = 2*m_maxHypoStackSize-1;
    // add in room for stack diversity
      if (m_minHypoStackDiversity)
        toleratedSize += m_minHypoStackDiversity << StaticData::Instance().GetMaxDistortion();
      if (m_hypos.size() > toleratedSize)
      {
        PruneToSize(m_maxHypoStackSize);
      }
      else {
        VERBOSE(3,std::endl);
      }
    } 
  
    return ret;
  }

  bool HypothesisStackRandom::AddPrune(Hypothesis *hypo)
  { 
  // too bad for stack. don't bother adding hypo into collection
    if (GetTotalScore(hypo) < m_worstScore
        && ! ( m_minHypoStackDiversity > 0
        && GetTotalScore(hypo) >= GetWorstScoreForBitmap( hypo->GetWordsBitmap() ) ) )
    {
      StaticData::Instance().GetSentenceStats().AddDiscarded();
      VERBOSE(3,"discarded, too bad for stack" << std::endl);
      FREEHYPO(hypo);   
      return false;
    }

  // over threshold, try to add to collection
    std::pair<iterator, bool> addRet = Add(hypo); 
    if (addRet.second)
    { // nothing found. add to collection
      return true;
    }

  // equiv hypo exists, recombine with other hypo
    iterator &iterExisting = addRet.first;
    Hypothesis *hypoExisting = *iterExisting;
    assert(iterExisting != m_hypos.end());

    StaticData::Instance().GetSentenceStats().AddRecombination(*hypo, **iterExisting);
  
  // found existing hypo with same target ending.
  // keep the best 1
    if (GetTotalScore(hypo) > GetTotalScore(hypoExisting))
    { // incoming hypo is better than the one we have
      VERBOSE(3,"better than matching hyp " << hypoExisting->GetId() << ", recombining, ");
    
      hypo->UpdateSpans(hypoExisting);
      if (m_nBestIsEnabled) {
        hypo->AddArc(hypoExisting);
        Detach(iterExisting);
      } else {
        Remove(iterExisting);
      }

      bool added = Add(hypo).second;    
      if (!added)
      {
        iterExisting = m_hypos.find(hypo);
        TRACE_ERR("Offending hypo = " << **iterExisting << endl);
        abort();
      }
      return false;
    }
    else
    { // already storing the best hypo. discard current hypo 
      VERBOSE(3,"worse than matching hyp " << hypoExisting->GetId() << ", recombining" << std::endl)
    
          hypoExisting->UpdateSpans(hypo);
      if (m_nBestIsEnabled) {
        hypoExisting->AddArc(hypo);
      } else {
        FREEHYPO(hypo);       
      }
      return false;
    }
  }

  void HypothesisStackRandom::PruneToSize(size_t newSize)
  {
    if ( size() <= newSize ) return; // ok, if not over the limit

  // we need to store a temporary list of hypotheses
    vector< Hypothesis* > hypos = GetSortedListNOTCONST();
    bool* included = (bool*) malloc(sizeof(bool) * hypos.size());
    for(size_t i=0; i<hypos.size(); i++) included[i] = false;

  // clear out original set
    for( iterator iter = m_hypos.begin(); iter != m_hypos.end(); ) 
    {
      iterator removeHyp = iter++;
      Detach(removeHyp);
    }

  // add best hyps for each coverage according to minStackDiversity
    if ( m_minHypoStackDiversity > 0 ) 
    {
      map< WordsBitmapID, size_t > diversityCount;
      for(size_t i=0; i<hypos.size(); i++) 
      {
        Hypothesis *hyp = hypos[i];
        WordsBitmapID coverage = hyp->GetWordsBitmap().GetID();;
        if (diversityCount.find( coverage ) == diversityCount.end()) 
          diversityCount[ coverage ] = 0;

        if (diversityCount[ coverage ] < m_minHypoStackDiversity) 
        {
          m_hypos.insert( hyp );
          included[i] = true;
          diversityCount[ coverage ]++;
          if (diversityCount[ coverage ] == m_minHypoStackDiversity)
            SetWorstScoreForBitmap( coverage, GetTotalScore(hyp));
        }
      }
    }

  // only add more if stack not full after satisfying minStackDiversity
    if ( size() < newSize ) {

    // add best remaining hypotheses
      for(size_t i=0; i<hypos.size() 
          && size() < newSize 
              && GetTotalScore(hypos[i]) > m_bestScore+m_beamWidth; i++)
      {
        if (! included[i]) 
        {
          m_hypos.insert( hypos[i] );
          included[i] = true;
          if (size() == newSize) 
            m_worstScore = GetTotalScore(hypos[i]);
        }
      }
    }

  // delete hypotheses that have not been included
    for(size_t i=0; i<hypos.size(); i++) 
    {
      if (! included[i])
      {
        FREEHYPO( hypos[i] );
        StaticData::Instance().GetSentenceStats().AddPruning();
      }
    }
    free(included);

  // some reporting....
    VERBOSE(3,", pruned to size " << size() << endl);
    IFVERBOSE(3) 
    {
      TRACE_ERR("stack now contains: ");
      for(iterator iter = m_hypos.begin(); iter != m_hypos.end(); iter++) 
      {
        Hypothesis *hypo = *iter;
        TRACE_ERR( hypo->GetId() << " (" << GetTotalScore(hypo) << ") ");
      }
      TRACE_ERR( endl);
    }
  }

  const Hypothesis *HypothesisStackRandom::GetBestHypothesis() const
  {
  
    unsigned long totalSpans = 0;
    if (!m_hypos.empty())
    {
      const_iterator iter = m_hypos.begin();
      Hypothesis *bestHypo = *iter;
      while (++iter != m_hypos.end())
      {
        Hypothesis *hypo = *iter;
        totalSpans += hypo->GetNumSpans();
        if (GetTotalScore(hypo) > GetTotalScore(bestHypo))
          bestHypo = hypo;
      }
      VERBOSE(1, "Total number of completed hyps considered " << totalSpans << endl);
      return bestHypo;
    }
    return NULL;
  }

  vector<const Hypothesis*> HypothesisStackRandom::GetSortedList() const
  {
    cerr << "Not implemented" << endl;
    assert(0);
    vector<const Hypothesis*> ret; ret.reserve(m_hypos.size());
    //    std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(ret, ret.end()));
    //sort(ret.begin(), ret.end(), CompareHypothesisTotalScore(this));

    return ret;
  }

  vector<Hypothesis*> HypothesisStackRandom::GetSortedListNOTCONST()
  {
    vector<Hypothesis*> ret; ret.reserve(m_hypos.size());
    std::copy(m_hypos.begin(), m_hypos.end(), std::inserter(ret, ret.end()));
    sort(ret.begin(), ret.end(), CompareHypothesisTotalScore(this));

    return ret;
  }

  void HypothesisStackRandom::CleanupArcList()
  {
  // only necessary if n-best calculations are enabled
    if (!m_nBestIsEnabled) return;

    iterator iter;
    for (iter = m_hypos.begin() ; iter != m_hypos.end() ; ++iter)
    {
      Hypothesis *mainHypo = *iter;
      mainHypo->CleanupArcList();
    }
  }
  
  float HypothesisStackRandom::GetTotalScore(const Hypothesis* hypo) {
    map<const Hypothesis*,float>::iterator si = m_scores.find(hypo);
    if (si == m_scores.end()) {
      float score = (float)RandomNumberGenerator::instance().next();
      m_scores[hypo] = score;
    }
    return m_scores[hypo];
  }
  
  float HypothesisStackRandom::GetTotalScore(const Hypothesis* hypo) const {
    return m_scores.find(hypo)->second;
  }

  TO_STRING_BODY(HypothesisStackRandom);


// friend
  std::ostream& operator<<(std::ostream& out, const HypothesisStackRandom& hypoColl)
  {
    HypothesisStackRandom::const_iterator iter;
  
    for (iter = hypoColl.begin() ; iter != hypoColl.end() ; ++iter)
    {
      const Hypothesis &hypo = **iter;
      out << hypo << endl;
    
    }
    return out;
  }


}
