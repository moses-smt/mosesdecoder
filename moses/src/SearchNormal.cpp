
#include "Timer.h"
#include "SearchNormal.h"

namespace Moses
{
#undef DEBUGLATTICE
#ifdef DEBUGLATTICE
static bool debug2 = false;
#endif

SearchNormal::SearchNormal(const InputType &source, const TranslationOptionCollection &transOptColl)
	:m_source(source)
	,m_hypoStackColl(source.GetSize() + 1)
	,m_initialTargetPhrase(Output)
	,m_start(clock())
	,interrupted_flag(0)
	,m_transOptColl(transOptColl)
{
	VERBOSE(1, "Translating: " << m_source << endl);
	const StaticData &staticData = StaticData::Instance();

	long sentenceID = source.GetTranslationId();
	m_constraint = staticData.GetConstrainingPhrase(sentenceID);

	std::vector < HypothesisStackNormal >::iterator iterStack;
	for (size_t ind = 0 ; ind < m_hypoStackColl.size() ; ++ind)
	{
		HypothesisStackNormal *sourceHypoColl = new HypothesisStackNormal();
		sourceHypoColl->SetMaxHypoStackSize(staticData.GetMaxHypoStackSize());
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
	staticData.ResetSentenceStats(m_source);
	const vector <DecodeGraph*>
		&decodeStepVL = staticData.GetDecodeStepVL();

	// initial seed hypothesis: nothing translated, no words produced
	Hypothesis *hypo = Hypothesis::Create(m_source, m_initialTargetPhrase);
	m_hypoStackColl[0]->AddPrune(hypo);

	// go through each stack
	std::vector < HypothesisStack* >::iterator iterStack;
	for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack)
	{

		//checked if elapsed time ran out of time with respect 
		double _elapsed_time = GetUserTime();
		if (_elapsed_time > staticData.GetTimeoutThreshold()){
			VERBOSE(1,"Decoding is out of time (" << _elapsed_time << "," << staticData.GetTimeoutThreshold() << ")" << std::endl);
			interrupted_flag = 1;
			return;
		}
		HypothesisStackNormal &sourceHypoColl = *static_cast<HypothesisStackNormal*>(*iterStack);

		// the stack is pruned before processing (lazy pruning):
		VERBOSE(3,"processing hypothesis from next stack");
		// VERBOSE("processing next stack at ");
		sourceHypoColl.PruneToSize(staticData.GetMaxHypoStackSize());
		VERBOSE(3,std::endl);
		sourceHypoColl.CleanupArcList();
		// go through each hypothesis on the stack and try to expand it
		HypothesisStackNormal::const_iterator iterHypo;
		for (iterHypo = sourceHypoColl.begin() ; iterHypo != sourceHypoColl.end() ; ++iterHypo)
		{
			Hypothesis &hypothesis = **iterHypo;
			ProcessOneHypothesis(hypothesis); // expand the hypothesis
		}
		// some logging
		IFVERBOSE(2) { OutputHypoStackSize(); }

		//This stack is fully expanded;
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
void SearchNormal::ProcessOneHypothesis(const Hypothesis &hypothesis)
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
				if (!hypoBitmap.Overlap(WordsRange(startPos, endPos)))
				{
					//TODO: does this method include incompatible WordLattice hypotheses?
					ExpandAllHypotheses(hypothesis
							, m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos)));
				}
			}
		}

		return; // done with special case (no reordering limit)
	}

	// if there are reordering limits, make sure it is not violated
	// the coverage bitmap is handy here (and the position of the first gap)
	const WordsBitmap hypoBitmap = hypothesis.GetWordsBitmap();
	const size_t	hypoFirstGapPos	= hypoBitmap.GetFirstGapPos()
		, sourceSize			= m_source.GetSize();

	// MAIN LOOP. go through each possible hypo
	for (size_t startPos = hypoFirstGapPos ; startPos < sourceSize ; ++startPos)
	{
		size_t maxSize = sourceSize - startPos;
		size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
#ifdef DEBUGLATTICE
		const int INTEREST = 29;
#endif
		maxSize = (maxSize < maxSizePhrase) ? maxSize : maxSizePhrase;
		size_t closestLeft = hypoBitmap.GetEdgeToTheLeftOf(startPos);
		if (isWordLattice) {
			// first question: is there a path from the closest translated word to the left
			// of the hypothesized extension to the start of the hypothesized extension?
			// long version: is there anything to our left? is it farther left than where we're starting anyway? can we get to it?
				//closestLeft is exclusive: a value of 3 means 2 is covered, our arc is currently ENDING at 3 and can start at 3 implicitly
			//if (closestLeft != startPos && closestLeft != 0 && ((startPos - closestLeft) != 1 && !m_source.CanIGetFromAToB(closestLeft+1, startPos+1))) {
			//if (closestLeft != startPos && closestLeft != 0 && ((startPos - closestLeft) != 0 && !m_source.CanIGetFromAToB(closestLeft, startPos))) {
			if (closestLeft != 0 && closestLeft != startPos && !m_source.CanIGetFromAToB(closestLeft, startPos)) {
#ifdef DEBUGLATTICE
				if (startPos == INTEREST) {
					std::cerr << "Die0: hyp = " << hypothesis <<"\n";
					std::cerr << "Die0: CanIGetFromAToB(" << (closestLeft) << "," << (startPos) << ") = " << m_source.CanIGetFromAToB(closestLeft,startPos) << "\n";
				}
#endif
				continue;
			}
		}
		//if (startPos == INTEREST) { std::cerr << "INTEREST: " << hypothesis << "\n"; }

		for (size_t endPos = startPos ; endPos < startPos + maxSize ; ++endPos)
		{
			// check for overlap
			WordsRange extRange(startPos, endPos);
#ifdef DEBUGLATTICE
			//if (startPos == INTEREST) { std::cerr << "  (" << hypoFirstGapPos << ")-> wr: " << extRange << "\n"; }
			bool interest = ((startPos == endPos) && (startPos == INTEREST));// || (startPos ==0 && endPos == 1);
			bool debug = interest;
			//(startPos > (INTEREST-8) && hypoFirstGapPos > 0 && startPos <= INTEREST && endPos >=INTEREST && endPos < (INTEREST+25) && hypoFirstGapPos == INTEREST);
			//debug2 = debug;
			//if (debug) { std::cerr << (startPos==INTEREST? "LOOK-->" : "") << "XP: " << hypothesis << "\next: " << extRange << "\n"; }
#endif
			if (hypoBitmap.Overlap(extRange) ||
					(isWordLattice && !m_source.IsCoveragePossible(extRange) 
						//||   !m_source.IsExtensionPossible(hypothesis.GetCurrSourceWordsRange(), extRange))
						//isExtensionPossible is redundant. We just need to know 1. is it consistant? 2. can our end touch the next thing to the right.
					)
			   )
			{
#ifdef DEBUGLATTICE
				if (interest) {
					//std::cerr << "Die1: hyp: " << hypothesis << "\n";
					//std::cerr << "Die1: extRange: " << extRange << "\n";
					//std::cerr << "Die1: currSrcRange: " << hypothesis.GetCurrSourceWordsRange() << "\n";
					//std::cerr << "Die1: Overlap=" << hypoBitmap.Overlap(extRange) << " IsCoveragePossible=" << m_source.IsCoveragePossible(extRange) << " IsExtensionPossible=" << m_source.IsExtensionPossible(hypothesis.GetCurrSourceWordsRange(), extRange)<< "\n";
				}
#endif
				continue;
			}
			bool leftMostEdge = (hypoFirstGapPos == startPos);
			size_t closestRight = hypoBitmap.GetEdgeToTheRightOf(endPos);	
			// ask second question here:
			// we already know we can get to our starting point from the closest thing to the left. We now ask the follow up:
			// can we get from our end to the closest thing on the right?
			// long version: is anything to our right? is it farther right than our (inclusive) end? can our end reach it?
			// closest right definition: 
			
			if (isWordLattice) {
				//if (!leftMostEdge && closestRight != endPos && closestRight != sourceSize && !m_source.CanIGetFromAToB(endPos, closestRight + 1)) {
				if (closestRight != endPos && ((closestRight + 1) < sourceSize) && !m_source.CanIGetFromAToB(endPos, closestRight + 1)) {
#ifdef DEBUGLATTICE
					if (interest) { 
						std::cerr << "Die2: hyp = " << hypothesis <<"\n";
						std::cerr << "Die2: extRange = " << extRange <<"\n";
						std::cerr << "Die2: CloseRight = " << closestRight <<"\n";
						std::cerr << "Die2: CanIGetFromAToB(" << (endPos) << "," << (closestRight+1) << ") = " << m_source.CanIGetFromAToB(endPos,closestRight + 1) << "\n";
					}
#endif
					continue;
				}
			}

			// any length extension is okay if starting at left-most edge
			if (leftMostEdge)
			{
#ifdef DEBUGLATTICE
				size_t vl = StaticData::Instance().GetVerboseLevel();
				if (debug2) {
					//std::cerr << "Ext!\n";
					StaticData::Instance().SetVerboseLevel(4);
				}
#endif
				ExpandAllHypotheses(hypothesis
						,m_transOptColl.GetTranslationOptionList(extRange));
#ifdef DEBUGLATTICE
				StaticData::Instance().SetVerboseLevel(vl);
#endif
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

				if (required_distortion <= maxDistortion) {
					ExpandAllHypotheses(hypothesis
							,m_transOptColl.GetTranslationOptionList(extRange));
				}
#ifdef DEBUGLATTICE
				else {
					if (interest) { 
						std::cerr << "Distortion violation\n"; 
						std::cerr << (startPos==INTEREST? "VLOOK-->" : "") << "XP: " << hypothesis << "\next: " << extRange << "\n";
					}
				}
#endif
			}
		}
		}
	}


	/**
	 * Expand a hypothesis given a list of translation options
	 * \param hypothesis hypothesis to be expanded upon
	 * \param transOptList list of translation options to be applied
	 */

	void SearchNormal::ExpandAllHypotheses(const Hypothesis &hypothesis,const TranslationOptionList &transOptList)
	{
		TranslationOptionList::const_iterator iter;
		for (iter = transOptList.begin() ; iter != transOptList.end() ; ++iter)
		{
			ExpandHypothesis(hypothesis, **iter);
		}
	}

	/**
	 * Expand one hypothesis with a translation option.
	 * this involves initial creation, scoring and adding it to the proper stack
	 * \param hypothesis hypothesis to be expanded upon
	 * \param transOpt translation option (phrase translation) 
	 *        that is applied to create the new hypothesis
	 */
	void SearchNormal::ExpandHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt) 
	{
		// create hypothesis and calculate all its scores
#ifdef DEBUGLATTICE
		if (debug2) { std::cerr << "::EXT: " << transOpt << "\n"; }
#endif
		Hypothesis *newHypo = hypothesis.CreateNext(transOpt, m_constraint); // TODO FIXME This is absolutely broken - don't pass null here

		if (newHypo==NULL) return;

		// expand hypothesis further if transOpt was linked
		for (std::vector<TranslationOption*>::const_iterator iterLinked = transOpt.GetLinkedTransOpts().begin();
				iterLinked != transOpt.GetLinkedTransOpts().end(); iterLinked++) {
			const WordsBitmap hypoBitmap = newHypo->GetWordsBitmap();
			if (hypoBitmap.Overlap((**iterLinked).GetSourceWordsRange())) {
				// don't want to add a hypothesis that has some but not all of a linked TO set, so return
				return;
			}
			else
			{
				newHypo->CalcScore(m_transOptColl.GetFutureScore());
				newHypo = newHypo->CreateNext(**iterLinked, m_constraint); // TODO FIXME This is absolutely broken - don't pass null here
			}
		}
		newHypo->CalcScore(m_transOptColl.GetFutureScore());

		// logging for the curious
		IFVERBOSE(3) {
			const StaticData &staticData = StaticData::Instance();
			newHypo->PrintHypothesis(m_source
					, staticData.GetWeightDistortion()
					, staticData.GetWeightWordPenalty());
		}

		// add to hypothesis stack
		size_t wordsTranslated = newHypo->GetWordsBitmap().GetNumWordsCovered();	
		m_hypoStackColl[wordsTranslated]->AddPrune(newHypo);
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
		//	const HypothesisStackNormal &hypoColl = m_hypoStackColl.back();
		if (interrupted_flag == 0){
			const HypothesisStackNormal &hypoColl = *static_cast<HypothesisStackNormal*>(m_hypoStackColl.back());
			return hypoColl.GetBestHypothesis();
		}
		else{
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
		for (++iterStack; iterStack != m_hypoStackColl.end() ; ++iterStack)
		{
			TRACE_ERR( ", " << (int)(*iterStack)->size());
		}
		TRACE_ERR( endl);
	}
 
}

