#include "Timer.h"
#include "SearchMulti.h"

namespace Moses
{
	/**
	 * Organizing main function
	 *
	 * /param source input sentence
	 * /param transOptColl collection of translation options to be used for this sentence
	 */
	SearchMulti::SearchMulti(const InputType &source, const InputType &source1, const TranslationOptionCollection &transOptColl, const TranslationOptionCollection &transOptColl1)
	:m_source(source)
	,m_source1(source1)
	//,m_hypoStackColl((source.GetSize() + 1) * (source1.GetSize() + 1))
	,m_hypoStackColl((source.GetSize() + 1))
	,m_initialTargetPhrase(Output)
	,m_start(clock())
	,interrupted_flag(0)
	,m_transOptColl(transOptColl)
	,m_transOptColl1(transOptColl1)
	{
		VERBOSE(1, "Translating: " << m_source <<
				" ...and also: " << m_source1 << endl);
		const StaticData &staticData = StaticData::Instance();
		
		// only if constraint decoding (having to match a specified output)
		long sentenceID = source.GetTranslationId();
		m_constraint = staticData.GetConstrainingPhrase(sentenceID);
		m_WERLimit = staticData.GetWERLimit();
		if (m_WERLimit < 0.0f) m_WERUnlimited = true;
		else m_WERUnlimited = false;
		
		// initialize the stacks: create data structure and set limits
		std::vector < HypothesisStackNormal >::iterator iterStack;
		for (size_t ind = 0 ; ind < m_hypoStackColl.size() ; ++ind)
		{
			HypothesisStackNormal *sourceHypoColl = new HypothesisStackNormal();
			sourceHypoColl->SetMaxHypoStackSize(staticData.GetMaxHypoStackSize(),staticData.GetMinHypoStackDiversity());
			sourceHypoColl->SetBeamWidth(staticData.GetBeamWidth());
			
			m_hypoStackColl[ind] = sourceHypoColl;
		}
	}
	
	SearchMulti::~SearchMulti()
	{
		RemoveAllInColl(m_hypoStackColl);
	}
	
	/**
	 * Main decoder loop that translates a sentence by expanding
	 * hypotheses stack by stack, until the end of the sentence.
	 */
	void SearchMulti::ProcessSentence()
	{
		/*
		 if (m_constraint!=NULL && m_WERUnlimited) {
		 // If attempting constraint decoding with unlimited WER allowed,
		 //    keep increasing allowed WER until a result is obtained.
		 for (m_WERLimit=0; GetBestHypothesis()==NULL; m_WERLimit++) {
		 VERBOSE(1, "WER Limit = " << m_WERLimit << endl);
		 AttemptProcessSentence();
		 }
		 //VERBOSE(1, "WER Limit = " << m_WERLimit << "  GetBestHypothesis()==" << *GetBestHypothesis() << endl);
		 } else {
		 AttemptProcessSentence();
		 }
		 */
		AttemptProcessSentence();
	}
	
	
	void SearchMulti::AttemptProcessSentence()
	{
		const StaticData &staticData = StaticData::Instance();
		SentenceStats &stats = staticData.GetSentenceStats();
		clock_t t=0; // used to track time for steps
		
		// initial seed hypothesis: nothing translated, no words produced
		Hypothesis *hypo = Hypothesis::Create(m_source, m_initialTargetPhrase);
		m_hypoStackColl[0]->AddPrune(hypo);
		
		// go through each stack
		std::vector < HypothesisStack* >::iterator iterStack;
		for (iterStack = m_hypoStackColl.begin() ; iterStack != m_hypoStackColl.end() ; ++iterStack)
		{
			// check if decoding ran out of time
			double _elapsed_time = GetUserTime();
			if (_elapsed_time > staticData.GetTimeoutThreshold()){
				VERBOSE(1,"Decoding is out of time (" << _elapsed_time << "," << staticData.GetTimeoutThreshold() << ")" << std::endl);
				interrupted_flag = 1;
				return;
			}
			HypothesisStackNormal &sourceHypoColl = *static_cast<HypothesisStackNormal*>(*iterStack);
			
			// the stack is pruned before processing (lazy pruning):
			VERBOSE(3,"processing hypothesis from next stack");
			IFVERBOSE(2) { t = clock(); }
			sourceHypoColl.PruneToSize(staticData.GetMaxHypoStackSize());
			VERBOSE(3,std::endl);
			sourceHypoColl.CleanupArcList();
			IFVERBOSE(2) { stats.AddTimeStack( clock()-t ); }
			
			// go through each hypothesis on the stack and try to expand it
			HypothesisStackNormal::const_iterator iterHypo;
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
		IFVERBOSE(2) { staticData.GetSentenceStats().SetTimeTotal( clock()-m_start ); }
		VERBOSE(2, staticData.GetSentenceStats());
	}
	
	
	/** Find all translation options to expand one hypothesis, trigger expansion
	 * this is mostly a check for overlap with already covered words, and for
	 * violation of reordering limits.
	 * \param hypothesis hypothesis to be expanded upon
	 */
	void SearchMulti::ProcessOneHypothesis(const Hypothesis &hypothesis)
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
	
	void SearchMulti::ExpandAllHypotheses(const Hypothesis &hypothesis, size_t startPos, size_t endPos)
	{
		// early discarding: check if hypothesis is too bad to build
		// this idea is explained in (Moore&Quirk, MT Summit 2007)
		float expectedScore = 0.0f;
		if (StaticData::Instance().UseEarlyDiscarding())
		{
			// expected score is based on score of current hypothesis
			expectedScore = hypothesis.GetScore();
			
			// add new future score estimate
			expectedScore += m_transOptColl.GetFutureScore().CalcFutureScore( hypothesis.GetWordsBitmap(), startPos, endPos );
		}
		
		//bool foundSomething = false;
		
		// loop through all translation options
		const TranslationOptionList &transOptList = m_transOptColl.GetTranslationOptionList(WordsRange(startPos, endPos));
		TranslationOptionList::const_iterator iter;
		for (iter = transOptList.begin() ; iter != transOptList.end() ; ++iter)
		{
			//LS		ExpandHypothesis(hypothesis, **iter, expectedScore);
			if (m_constraint == NULL) {
				ExpandHypothesis(hypothesis, **iter, expectedScore);
			}
			else if (m_constraint != NULL && m_WERLimit == 0.0f) {
				if (isCompatibleWithConstraint(hypothesis, **iter) ) {
					//(**iter).
					ExpandHypothesis(hypothesis, **iter, expectedScore);
					//foundSomething = true;
				} //else {
				//	VERBOSE(1,"Expanding incompatible hypothesis" << endl);
				//	 ExpandHypothesis(hypothesis, **iter, expectedScore);
				//}
				
			} else {
				TargetPhrase *curTarget = getCurrentTargetPhrase(hypothesis);
				//LS			float currConstraintWER = getCurrConstraintWER(hypothesis, **iter);
				float currConstraintWER = getCurrConstraintWER(curTarget, **iter);
				//LS			VERBOSE(1, "WER==" << currConstraintWER << " for \"" << static_cast<const Phrase&>(*curTarget) << "\"" << endl);
				//printf("WER: %f  Limit: %f\n", currConstraintWER, m_WERLimit);
				if (currConstraintWER <= m_WERLimit)
					ExpandHypothesis(hypothesis, **iter, expectedScore);
			}
		}
		/*
		 if (m_constraint!=NULL && !foundSomething) {
		 size_t start = 1 + hypothesis.GetCurrTargetWordsRange().GetEndPos();
		 const WordsRange range(start, start);
		 const Phrase &relevantConstraint = m_constraint->GetSubString(range);
		 //const TargetPhrase tp(relevantConstraint);
		 Phrase sourcePhrase(Input);
		 std::string targetPhraseString("hi");
		 
		 vector<FactorType> 	output	= Tokenize<FactorType>("0,", ",");
		 //const vector<FactorType> 	output();
		 const StaticData &staticData = StaticData::Instance();
		 TargetPhrase targetPhrase(Output);
		 //targetPhrase.AddWord(<#const Word newWord#>)
		 targetPhrase.SetSourcePhrase(&sourcePhrase);
		 targetPhrase.CreateFromString( output, targetPhraseString, staticData.GetFactorDelimiter());
		 
		 TranslationOption newOpt(range, targetPhrase, m_source);
		 
		 
		 VERBOSE(2, "Should we add \"" << relevantConstraint << "\" (" << start << "-" << start << ") " << newOpt << endl);
		 
		 ExpandHypothesis(hypothesis, newOpt, expectedScore);
		 }
		 VERBOSE(2, "Found something==" << foundSomething << " for " << startPos << "-" << endPos << endl);
		 */
	}
	
	/**
	 * Enforce constraint when appropriate
	 * \param hypothesis hypothesis to be expanded upon
	 * \param transOptList list of translation options to be applied
	 */
	
	bool SearchMulti::isCompatibleWithConstraint(const Hypothesis &hypothesis, 
												 const TranslationOption &transOpt) 
	{
		size_t constraintSize = m_constraint->GetSize();
		size_t start = 1 + hypothesis.GetCurrTargetWordsRange().GetEndPos();
		const Phrase &transOptPhrase = transOpt.GetTargetPhrase();
		size_t transOptSize = transOptPhrase.GetSize();
		
		if (transOptSize==0) {
			VERBOSE(4, "Empty transOpt IS COMPATIBLE with constraint \"" <<  *m_constraint << "\"" << endl);
			return true;
		}
		size_t endpoint = start + transOptSize - 1;
		//size_t endpoint = start + transOptSize;
		//if (endpoint > 0) endpoint = endpoint - 1;
		WordsRange range(start, endpoint);
		
		if (endpoint >= constraintSize) {
			VERBOSE(4, "Appending \"" << transOptPhrase << "\" after \"" << static_cast<const Phrase&>(hypothesis.GetTargetPhrase()) << "\" (start=" << start << ", endpoint=" << endpoint << ", transOptSize=" << transOptSize << ") would be too long for constraint \"" <<  *m_constraint << "\"" << endl);
			return false;
		} else {
			const Phrase &relevantConstraint = m_constraint->GetSubString(range);
			if ( ! relevantConstraint.IsCompatible(transOptPhrase) ) {
				VERBOSE(4, "\"" << transOptPhrase << "\" is incompatible with \"" <<  relevantConstraint << "\" (" << start << "-" << endpoint << ")" << endl);
				return false;
			} else {
				VERBOSE(4, "\"" << transOptPhrase << "\" IS COMPATBILE with \"" <<  relevantConstraint << "\"" << endl);
				return true;
			}
		}
	}
	
	TargetPhrase *SearchMulti::getCurrentTargetPhrase(const Hypothesis &hypothesis) 
	{ 
		// Rebuild Target String via recursing on previous hypothesis
		const Hypothesis *hypo = &hypothesis;
		std::vector<Phrase> target;
		
		while (hypo != NULL) {
			target.push_back(hypo->GetCurrTargetPhrase());
			hypo = hypo->GetPrevHypo();	
		}
		
		TargetPhrase *targetphrase = new TargetPhrase();
		
		for (int i = target.size() - 1; i >= 0; i--) {
			targetphrase->Append(target[i]);
		}
		
		return targetphrase;
	}
	
	float SearchMulti::getCurrConstraintWER(TargetPhrase *curTarget, 
											const TranslationOption &transOpt) 
	{
		
		//const size_t constraintSize = m_constraint->GetSize();
		const TargetPhrase transOptPhrase = transOpt.GetTargetPhrase();
		
		
		TargetPhrase newTarget = TargetPhrase(*curTarget);
		newTarget.Append(transOptPhrase);
		
		//size_t endpoint = newTarget.GetSize() - 1;
		
		// Account for target strings that are longer than the reference
		//if (endpoint >= constraintSize)
		//	endpoint = constraintSize - 1;
		
		// Extract relevant constraint...
		//WordsRange range(0, endpoint);
		//const Phrase &relevantConstraint = m_constraint->GetSubString(range);
		
		
		// Compute WER between reference and target string
		//float editDistance = computeEditDistance(relevantConstraint, newTarget);
		float editDistance = computeEditDistance(*m_constraint, newTarget);
		float normalizedEditDistance = editDistance - (m_constraint->GetSize() - newTarget.GetSize());
		normalizedEditDistance = (normalizedEditDistance<0) ? editDistance : normalizedEditDistance;
		//VERBOSE(1, "m_constraint->GetSize() - newTarget.GetSize() == " << m_constraint->GetSize() << " - " << newTarget.GetSize() << endl);
		VERBOSE(2, "WER==" << normalizedEditDistance << " (" << editDistance << ") for \"" << static_cast<const Phrase&>(newTarget) << "\" with constraint \"" << *m_constraint << "\"" << endl);
		
		//return editDistance;
		return normalizedEditDistance;
	}
	
	
	float SearchMulti::computeEditDistance(const Phrase &hypPhrase, const Phrase &constraintPhrase) const
	{
		const size_t len1 = hypPhrase.GetSize(), len2 = constraintPhrase.GetSize();
		vector<vector<unsigned int> > d(len1 + 1, vector<unsigned int>(len2 + 1));
		
		for(int i = 0; i <= len1; ++i) d[i][0] = i;
		for(int i = 0; i <= len2; ++i) d[0][i] = i;
		
		for(int i = 1; i <= len1; ++i)
		{
			for(int j = 1; j <= len2; ++j) {
				WordsRange s1range(i-1, i-1);
				WordsRange s2range(j-1, j-1);
				int cost = hypPhrase.GetSubString(s1range).IsCompatible(constraintPhrase.GetSubString(s2range)) ? 0 : 1;
				d[i][j] = std::min( std::min(d[i - 1][j] + 1,
											 d[i][j - 1] + 1),
								   d[i - 1][j - 1] + cost);
			}
		}
		return d[len1][len2];
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
	void SearchMulti::ExpandHypothesis(const Hypothesis &hypothesis, const TranslationOption &transOpt, float expectedScore)
	{
		const StaticData &staticData = StaticData::Instance();
		SentenceStats &stats = staticData.GetSentenceStats();
		clock_t t=0; // used to track time for steps
		
		Hypothesis *newHypo;
		if (! staticData.UseEarlyDiscarding())
		{
			// simple build, no questions asked
			IFVERBOSE(2) { t = clock(); }
			//LS		newHypo = hypothesis.CreateNext(transOpt, m_constraint);
			newHypo = hypothesis.CreateNext(transOpt);
			IFVERBOSE(2) { stats.AddTimeBuildHyp( clock()-t ); }
			if (newHypo==NULL) return;
			newHypo->CalcScore(m_transOptColl.GetFutureScore());
			//newHypo->IncrementTotalScore(bonus);
		}
		else
			// early discarding: check if hypothesis is too bad to build
		{
			// worst possible score may have changed -> recompute
			size_t wordsTranslated = hypothesis.GetWordsBitmap().GetNumWordsCovered() + transOpt.GetSize();
			float allowedScore = m_hypoStackColl[wordsTranslated]->GetWorstScore();
			if (staticData.GetMinHypoStackDiversity())
			{
				WordsBitmapID id = hypothesis.GetWordsBitmap().GetIDPlus(transOpt.GetStartPos(), transOpt.GetEndPos());
				float allowedScoreForBitmap = m_hypoStackColl[wordsTranslated]->GetWorstScoreForBitmap( id );
				allowedScore = std::min( allowedScore, allowedScoreForBitmap );
			}
			allowedScore += staticData.GetEarlyDiscardingThreshold();
			
			// add expected score of translation option
			expectedScore += transOpt.GetFutureScore();
			// TRACE_ERR("EXPECTED diff: " << (newHypo->GetTotalScore()-expectedScore) << " (pre " << (newHypo->GetTotalScore()-expectedScorePre) << ") " << hypothesis.GetTargetPhrase() << " ... " << transOpt.GetTargetPhrase() << " [" << expectedScorePre << "," << expectedScore << "," << newHypo->GetTotalScore() << "]" << endl);
			//expectedScore += bonus;
			// check if transOpt score push it already below limit
			if (expectedScore < allowedScore)
			{
				IFVERBOSE(2) { stats.AddNotBuilt(); }
				return;
			}
			
			// build the hypothesis without scoring
			IFVERBOSE(2) { t = clock(); }
			//LS		newHypo = hypothesis.CreateNext(transOpt, m_constraint);
			newHypo = hypothesis.CreateNext(transOpt);
			if (newHypo==NULL) return;
			IFVERBOSE(2) { stats.AddTimeBuildHyp( clock()-t ); }
			
			// compute expected score (all but correct LM)
			expectedScore = newHypo->CalcExpectedScore( m_transOptColl.GetFutureScore() );
			
			// ... and check if that is below the limit
			if (expectedScore < allowedScore)
			{
				IFVERBOSE(2) { stats.AddEarlyDiscarded(); }
				FREEHYPO( newHypo );
				return;
			}
			
			// ok, all is good, compute remaining scores
			newHypo->CalcRemainingScore();
			//newHypo->IncrementTotalScore(bonus);
		}
		
		// logging for the curious
		IFVERBOSE(3) {
			newHypo->PrintHypothesis();
		}
		
		// add to hypothesis stack
		size_t wordsTranslated = newHypo->GetWordsBitmap().GetNumWordsCovered();
		IFVERBOSE(2) { t = clock(); }
		m_hypoStackColl[wordsTranslated]->AddPrune(newHypo);
		IFVERBOSE(2) { stats.AddTimeStack( clock()-t ); }
	}
	
	const std::vector < HypothesisStack* >& SearchMulti::GetHypothesisStacks() const
	{
		return m_hypoStackColl;
	}
	
	/**
	 * Find best hypothesis on the last stack.
	 * This is the end point of the best translation, which can be traced back from here
	 */
	const Hypothesis *SearchMulti::GetBestHypothesis() const
	{
		/*LS
		 if (interrupted_flag == 0){
		 const HypothesisStackNormal &hypoColl = *static_cast<HypothesisStackNormal*>(m_hypoStackColl.back());
		 return hypoColl.GetBestHypothesis();
		 }
		 else{
		 const HypothesisStackNormal &hypoColl = *actual_hypoStack;
		 return hypoColl.GetBestHypothesis();
		 }
		 */
		
		if (interrupted_flag == 0){
			const HypothesisStackNormal &hypoColl = *static_cast<HypothesisStackNormal*>(m_hypoStackColl.back());
			
			if (m_constraint != NULL) {
				HypothesisStackNormal::const_iterator iter;
				
				const Hypothesis *bestHypo = NULL;
				
				
				for (iter = hypoColl.begin() ; iter != hypoColl.end() ; ++iter)
				{
					const Hypothesis *hypo = *iter;
					WordsRange range(0, m_constraint->GetSize() - 1);
					Phrase constraint = m_constraint->GetSubString(range);
					
					if (hypo != NULL) {
						TargetPhrase targetPhrase = TargetPhrase(hypo->GetCurrTargetPhrase());
						hypo = hypo->GetPrevHypo();
						while (hypo != NULL) {
							TargetPhrase newTargetPhrase = TargetPhrase(hypo->GetCurrTargetPhrase());
							newTargetPhrase.Append(targetPhrase);
							targetPhrase = newTargetPhrase;
							hypo = hypo->GetPrevHypo();
						}
						
						if ( m_WERLimit != 0.0f ) { // is WER-constraint active?
							//VERBOSE(1, "constraint  : " << constraint << endl);
							//VERBOSE(1, "targetPhrase: " << targetPhrase << endl);
							if (computeEditDistance(constraint, targetPhrase) <= m_WERLimit) {
								//VERBOSE(1, "TRUE" << endl);
								if (bestHypo==NULL || (*iter)->GetTotalScore() > bestHypo->GetTotalScore())
									bestHypo = *iter;
							} else {
								//VERBOSE(1, "FALSE" << endl);
							}
						} else {
							if (constraint.IsCompatible(targetPhrase) &&
								(bestHypo==NULL || (*iter)->GetTotalScore() > bestHypo->GetTotalScore()))
								bestHypo = *iter;
						}
					}
				}
				return bestHypo;
				//return NULL;
			} else {
				return hypoColl.GetBestHypothesis();
			}
		}
		else{
			const HypothesisStackNormal &hypoColl = *actual_hypoStack;
			return hypoColl.GetBestHypothesis();
		}
		
	}
	
	/**
	 * Logging of hypothesis stack sizes
	 */
	void SearchMulti::OutputHypoStackSize()
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

