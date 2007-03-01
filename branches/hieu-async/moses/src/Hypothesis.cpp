// $Id$
// vim:tabstop=2
/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "DummyScoreProducers.h"
#include "Hypothesis.h"
#include "Util.h"
#include "LexicalReordering.h"
#include "StaticData.h"
#include "InputType.h"
#include "LMList.h"
#include "hash.h"
#include "SpanScore.h"
#include "LanguageModelMultiFactor.h"

using namespace std;

unsigned int Hypothesis::s_HypothesesCreated = 0;

#ifdef USE_HYPO_POOL
	ObjectPool<Hypothesis> Hypothesis::s_objectPool("Hypothesis", 300000);
#endif

Hypothesis::Hypothesis(InputType const& source, const std::vector<DecodeStep*> &decodeStepList, const TargetPhrase &emptyTarget)
	: m_prevHypo(NULL)
	, m_backPtr(decodeStepList.size())
	, m_currSourceRange(decodeStepList.size())
	, m_currTargetPhrase(emptyTarget)
	, m_targetPhrase(emptyTarget)
	, m_targetSize(decodeStepList.size(), 0)
	, m_sourcePhrase(0)
	, m_sourceCompleted(source.GetSize())
	, m_sourceInput(source)
	, m_currTargetWordsRange()
	, m_wordDeleted(false)
	, m_languageModelStates(StaticData::Instance().GetLMSize(), LanguageModelSingleFactor::UnknownState)
	, m_arcList(NULL)
	, m_id(0)
	, m_lmstats(NULL)
	, m_decodeStepId(NOT_FOUND)
	, m_alignPair(source.GetSize())
{	// used for initial seeding of trans process	
	// initialize scores
	//_hash_computed = false;
	s_HypothesesCreated = 1;
	ResetScore();	

	for (size_t decodeStepId = 0 ; decodeStepId < m_backPtr.size() ; ++decodeStepId)
	{
		m_backPtr[decodeStepId] = NULL;
	}
}

/***
 * continue prevHypo by appending the phrases in transOpt
 */
Hypothesis::Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt)
	: m_prevHypo(&prevHypo)
	, m_backPtr(prevHypo.m_backPtr)
	, m_currSourceRange(prevHypo.m_currSourceRange)
	, m_currTargetPhrase(transOpt.GetTargetPhrase())
	, m_targetPhrase(prevHypo.m_targetPhrase)
	, m_targetSize(prevHypo.m_targetSize)
	, m_sourcePhrase(0)
	, m_sourceCompleted				(prevHypo.m_sourceCompleted )
	, m_sourceInput						(prevHypo.m_sourceInput)
	, m_currTargetWordsRange()
	, m_wordDeleted(false)
	,	m_totalScore(0.0f)
	,	m_futureScore(0.0f)
	, m_scoreBreakdown			(prevHypo.m_scoreBreakdown)
	, m_languageModelStates(prevHypo.m_languageModelStates)
	, m_arcList(NULL)
	, m_id(s_HypothesesCreated++)
	, m_lmstats(NULL)
	,	m_decodeStepId (transOpt.GetDecodeStepId())
	, m_alignPair(prevHypo.m_alignPair)
{
	const Phrase &transOptPhrase = transOpt.GetTargetPhrase();

	// update curr source range for this step
	m_currSourceRange[m_decodeStepId] = transOpt.GetSourceWordsRange();

	// update target size for this step
	m_targetSize[m_decodeStepId] += transOptPhrase.GetSize();

	// update back ptr
	size_t prevDecodeStepId = prevHypo.GetDecodeStepId();
	if (prevDecodeStepId != NOT_FOUND)
		m_backPtr[prevDecodeStepId] = &prevHypo;

	// update target range
	const Hypothesis *backPtrHypo = m_backPtr[m_decodeStepId];
	if (backPtrHypo == NULL)
	{
		m_currTargetWordsRange = WordsRange(
																transOpt.GetDecodeStepId()
																, 0
																, transOptPhrase.GetSize() - 1 );
	}
	else
	{
		size_t lastPos = backPtrHypo->GetCurrTargetWordsRange().GetEndPos();
		m_currTargetWordsRange = WordsRange(
																transOpt.GetDecodeStepId()
																, lastPos + 1
																, lastPos + transOptPhrase.GetSize());
	}

	// merge factors from new trans opt into taget phrase so far
	m_targetPhrase.MergeFactors(transOptPhrase, m_currTargetWordsRange);

	// update alignment
	m_alignPair.Merge(transOpt.GetAlignmentPair()
									, transOpt.GetSourceWordsRange()
									, m_currTargetWordsRange);

	// assert that we are not extending our hypothesis by retranslating something
	// that this hypothesis has already translated!
	assert(!m_sourceCompleted.Overlap(GetCurrSourceWordsRange()));	

	m_sourceCompleted.SetValue(transOpt.GetSourceWordsRange(), true);
  m_wordDeleted = transOpt.IsDeletionOption();
	m_scoreBreakdown.PlusEquals(transOpt.GetScoreBreakdown());

}

size_t Hypothesis::GetNextStartPos(const TranslationOption &transOpt) const
{
	// are we in the same decode step at the moment ?
	if (m_decodeStepId == transOpt.GetDecodeStepId())
		return GetCurrTargetWordsRange().GetEndPos() + 1;

	// update target range
	const Hypothesis *backPtrHypo = m_backPtr[transOpt.GetDecodeStepId()];
	if (backPtrHypo == NULL)
	{
		return 0;
	}
	else
	{
		size_t lastPos = backPtrHypo->GetCurrTargetWordsRange().GetEndPos();
		return lastPos + 1;
	}
}

Hypothesis::~Hypothesis()
{
	if (m_arcList) 
	{
		ArcList::iterator iter;
		for (iter = m_arcList->begin() ; iter != m_arcList->end() ; ++iter)
		{
			FREEHYPO(*iter);
		}
		m_arcList->clear();

		delete m_arcList;
		m_arcList = NULL;

		delete m_lmstats; m_lmstats = NULL;
	}
}

void Hypothesis::AddArc(Hypothesis *loserHypo)
{
	if (!m_arcList) {
		if (loserHypo->m_arcList)  // we don't have an arcList, but loser does
		{
			this->m_arcList = loserHypo->m_arcList;  // take ownership, we'll delete
			loserHypo->m_arcList = 0;                // prevent a double deletion
		}
		else
			{ this->m_arcList = new ArcList(); }
	} else {
		if (loserHypo->m_arcList) {  // both have an arc list: merge. delete loser
			size_t my_size = m_arcList->size();
			size_t add_size = loserHypo->m_arcList->size();
			this->m_arcList->resize(my_size + add_size, 0);
			std::memcpy(&(*m_arcList)[0] + my_size, &(*m_arcList)[0], add_size * sizeof(Hypothesis *));
			delete loserHypo->m_arcList;
			loserHypo->m_arcList = 0;
		} else { // loserHypo doesn't have any arcs
		  // DO NOTHING
		}
	}
	m_arcList->push_back(loserHypo);
}

/***
 * return the subclass of Hypothesis most appropriate to the given translation option
 */
Hypothesis* Hypothesis::CreateNext(const TranslationOption &transOpt) const
{
	return Create(*this, transOpt);
}

/***
 * return the subclass of Hypothesis most appropriate to the given translation option
 */
Hypothesis* Hypothesis::Create(const Hypothesis &prevHypo, const TranslationOption &transOpt)
{
#ifdef USE_HYPO_POOL
	Hypothesis *ptr = s_objectPool.getPtr();
	return new(ptr) Hypothesis(prevHypo, transOpt);
#else
	return new Hypothesis(prevHypo, transOpt);
#endif
}
/***
 * return the subclass of Hypothesis most appropriate to the given target phrase
 */

Hypothesis* Hypothesis::Create(InputType const& m_source, const std::vector<DecodeStep*> &decodeStepList, const TargetPhrase &emptyTarget)
{
#ifdef USE_HYPO_POOL
	Hypothesis *ptr = s_objectPool.getPtr();
	return new(ptr) Hypothesis(m_source, decodeStepList, emptyTarget);
#else
	return new Hypothesis(m_source, decodeStepList, emptyTarget);
#endif
}

int Hypothesis::CompareCurrSourceRange(const Hypothesis &compare) const
{
	for (size_t decodeStepId = 0 ; decodeStepId < m_currSourceRange.size() ; ++decodeStepId)
	{
		if (m_currSourceRange[decodeStepId].GetEndPos() < compare.m_currSourceRange[decodeStepId].GetEndPos())
			return -1;
		if (m_currSourceRange[decodeStepId].GetEndPos() > compare.m_currSourceRange[decodeStepId].GetEndPos())
			return +1;
	}

	if (! StaticData::Instance().GetSourceStartPosMattersForRecombination()) 
		return 0;

	for (size_t decodeStepId = 0 ; decodeStepId < m_currSourceRange.size() ; ++decodeStepId)
	{
		if (m_currSourceRange[decodeStepId].GetStartPos() < compare.m_currSourceRange[decodeStepId].GetStartPos())
			return -1;
		if (m_currSourceRange[decodeStepId].GetStartPos() > compare.m_currSourceRange[decodeStepId].GetStartPos())
			return +1;
	}

	return 0;
}

//helper fn - turn vector arg from vector of 
// sizes to vector of diff to the 0 element
void SizeDiff(vector<size_t> &phraseSize)
{
	const size_t numElem = phraseSize.size();
	for (size_t idx = 1 ; idx < numElem ; ++idx)
		phraseSize[idx] -= phraseSize[0];

	phraseSize[0] = 0;
}

int Hypothesis::CompareUnsyncFactors(const Hypothesis &compare) const
{
	std::vector<size_t> thisSizeDiff		= m_targetSize
										,compareSizeDiff	= compare.m_targetSize;
	SizeDiff(thisSizeDiff);
	SizeDiff(compareSizeDiff);

	if (thisSizeDiff == compareSizeDiff)
		return 0;

	return (thisSizeDiff < compareSizeDiff) ? +1 : -1;
}

/** Calculates the overall language model score by combining the scores
 * of language models generated for each of the factors.  Because the factors
 * represent a variety of tag sets, and because factors with smaller tag sets 
 * (such as POS instead of words) allow us to calculate richer statistics, we
 * allow a different length of n-gram to be specified for each factor.
 * /param languageModels - list of all language models
 */
void Hypothesis::CalcLMScore(const LMList &languageModels)
{
	const size_t startPos	= m_currTargetWordsRange.GetStartPos();
	LMList::const_iterator iterLM;

	// will be null if LM stats collection is disabled
	if (StaticData::Instance().IsComputeLMBackoffStats()) {
		m_lmstats = new vector<vector<unsigned int> >(languageModels.size(), vector<unsigned int>(0));
	}

	size_t lmIdx = 0;
	const DecodeStep &decodeStep = StaticData::Instance().GetDecodeStep(
																		m_currTargetWordsRange.GetDecodeStepId());
	const FactorMask &targetMask = decodeStep.GetOutputFactorMask();

	// already have LM scores from previous and trigram score of poss trans.
	// just need trigram score of the words of the start of current phrase	
	for (iterLM = languageModels.begin() ; iterLM != languageModels.end() ; ++iterLM,++lmIdx)
	{
		const LanguageModel &languageModel = **iterLM;
		if (languageModel.Useable(targetMask))
		{
			size_t nGramOrder			= languageModel.GetNGramOrder();
			size_t currEndPos			= m_currTargetWordsRange.GetEndPos();
			float lmScore;
			size_t nLmCallCount = 0;

			if(m_currTargetWordsRange.GetNumWordsCovered() == 0) {
				lmScore = 0; //the score associated with dropping source words is not part of the language model
			} else { //non-empty target phrase
				if (m_lmstats)
					(*m_lmstats)[lmIdx].resize(m_currTargetWordsRange.GetNumWordsCovered(), 0);

				// 1st n-gram
				vector<const Word*> contextFactor(nGramOrder);
				size_t index = 0;
				for (int currPos = (int) startPos - (int) nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
				{
					if (currPos >= 0)
						contextFactor[index++] = &GetWord(currPos);
					else			
						contextFactor[index++] = &languageModel.GetSentenceStartArray();
				}
				lmScore	= languageModel.GetValue(contextFactor);
				if (m_lmstats) { languageModel.GetState(contextFactor, &(*m_lmstats)[lmIdx][nLmCallCount++]); }

				// main loop
				size_t endPos = std::min(startPos + nGramOrder - 2
																, currEndPos);
				for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++)
				{
					// shift all args down 1 place
					for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
						contextFactor[i] = contextFactor[i + 1];
		
					// add last factor
					contextFactor.back() = &GetWord(currPos);

					lmScore	+= languageModel.GetValue(contextFactor);
					if (m_lmstats) 
						languageModel.GetState(contextFactor, &(*m_lmstats)[lmIdx][nLmCallCount++]);
					//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;		
				}

				// end of sentence
				if ( (languageModel.GetLMType() == SingleFactor 
							&& m_sourceCompleted.IsComplete(static_cast<const LanguageModelSingleFactor&>(languageModel).GetFactorType()))
							|| (languageModel.GetLMType() == MultiFactor 
							&& m_sourceCompleted.IsComplete(static_cast<const LanguageModelMultiFactor&>(languageModel).GetFactorMask()))
						)
				{
					const size_t size = GetSize();
					contextFactor.back() = &languageModel.GetSentenceEndArray();
		
					for (size_t i = 0 ; i < nGramOrder - 1 ; i ++)
					{
						int currPos = (int)(size - nGramOrder + i + 1);
						if (currPos < 0)
							contextFactor[i] = &languageModel.GetSentenceStartArray();
						else
							contextFactor[i] = &GetWord((size_t)currPos);
					}
					if (m_lmstats) 
					{
						(*m_lmstats)[lmIdx].resize((*m_lmstats)[lmIdx].size() + 1); // extra space for the last call
						lmScore += languageModel.GetValue(contextFactor, &m_languageModelStates[lmIdx], &(*m_lmstats)[lmIdx][nLmCallCount++]);
					}
					else
					{
						lmScore	+= languageModel.GetValue(contextFactor, &m_languageModelStates[lmIdx]);
					}
				}
				else
				{ // not yet end of sentence
					for (size_t currPos = endPos+1; currPos <= currEndPos; currPos++) {
						for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
							contextFactor[i] = contextFactor[i + 1];
						contextFactor.back() = &GetWord(currPos);
						if (m_lmstats)
							languageModel.GetState(contextFactor, &(*m_lmstats)[lmIdx][nLmCallCount++]);
					}
					m_languageModelStates[lmIdx]=languageModel.GetState(contextFactor);
				}
			}
			
			m_scoreBreakdown.PlusEquals(&languageModel, lmScore);
		} // if (languageModel.Useable())
	}
}

void Hypothesis::CalcDistortionScore()
{
	const DistortionScoreProducer *dsp = StaticData::Instance().GetDistortionScoreProducer();

	float distortionScore;
	if (m_prevHypo->GetDecodeStepId() == NOT_FOUND)
	{ // 1st translate phrase. prev hypo is seed hypo
		distortionScore = - (float) GetCurrSourceWordsRange().GetStartPos();
	}
	else
	{
		distortionScore = dsp->CalculateDistortionScore(
				m_prevHypo->GetCurrSourceWordsRange(),
				this->GetCurrSourceWordsRange()
			 );
	}
	m_scoreBreakdown.PlusEquals(dsp, distortionScore);
}

void Hypothesis::ResetScore()
{
	m_scoreBreakdown.ZeroAll();
	m_futureScore = m_totalScore = 0.0f;
}

/***
 * calculate the logarithm of our total translation score (sum up components)
 */
void Hypothesis::CalcScore(const SpanScore &futureScore) 
{
	const StaticData &staticData = StaticData::Instance();

	// DISTORTION COST
	CalcDistortionScore();
	
	// LANGUAGE MODEL COST
	CalcLMScore(staticData.GetAllLM());

	// WORD PENALTY
	m_scoreBreakdown.PlusEquals(staticData.GetWordPenaltyProducer(), - (float) m_currTargetWordsRange.GetNumWordsCovered()); 

	// FUTURE COST
	CalcFutureScore(futureScore);

	
	//LEXICAL REORDERING COST
	std::vector<LexicalReordering*> m_reorderModels = staticData.GetReorderModels();
	for(unsigned int i = 0; i < m_reorderModels.size(); i++)
	{
		m_scoreBreakdown.PlusEquals(m_reorderModels[i], m_reorderModels[i]->CalcScore(this));
	}

	// TOTAL
	m_totalScore = m_scoreBreakdown.InnerProduct(staticData.GetAllWeights()) + m_futureScore;
}

void Hypothesis::CalcFutureScore(const SpanScore &futureScore)
{
	// future cost of untranslated parts of source sentence
	m_futureScore = futureScore.GetFutureScore(m_sourceCompleted);

	// add future costs for distortion model
	if(StaticData::Instance().UseDistortionFutureCosts())
		m_futureScore += m_sourceCompleted.GetFutureDistortionScore((int) GetCurrSourceWordsRange().GetEndPos()) * StaticData::Instance().GetWeightDistortion();
	
}

const Hypothesis* Hypothesis::GetPrevHypo()const{
	return m_prevHypo;
}

/**
 * print hypothesis information for pharaoh-style logging
 */
void Hypothesis::PrintHypothesis(const InputType &source, float /*weightDistortion*/, float /*weightWordPenalty*/) const
{
  TRACE_ERR( "creating hypothesis "<< m_id <<" from "<< m_prevHypo->m_id<<" ( ");
  int end = (int)(m_prevHypo->m_currTargetPhrase.GetSize()-1);
  int start = end-1;
  if ( start < 0 ) start = 0;
  if ( m_prevHypo->m_currTargetWordsRange.GetStartPos() == NOT_FOUND ) {
    TRACE_ERR( "<s> ");
  }
  else {
    TRACE_ERR( "... ");
  }
  if (end>=0) {
    WordsRange range(0, start, end); // TODO - hack. 
    TRACE_ERR( m_prevHypo->m_currTargetPhrase.GetSubString(range) << " ");
  }
  TRACE_ERR( ")"<<endl);
	TRACE_ERR( "\tbase score "<< (m_prevHypo->m_totalScore - m_prevHypo->m_futureScore) <<endl);
	TRACE_ERR( "\tcovering "<< GetCurrSourceWordsRange().GetStartPos()<<"-"<<GetCurrSourceWordsRange().GetEndPos()<<": "<< source.GetSubString(GetCurrSourceWordsRange())  <<endl);
	TRACE_ERR( "\ttranslated as: "<<m_currTargetPhrase<<endl); // <<" => translation cost "<<m_score[ScoreType::PhraseTrans];
	if (m_wordDeleted) TRACE_ERR( "\tword deleted"<<endl); 
  //	TRACE_ERR( "\tdistance: "<<GetCurrSourceWordsRange().CalcDistortion(m_prevHypo->GetCurrSourceWordsRange())); // << " => distortion cost "<<(m_score[ScoreType::Distortion]*weightDistortion)<<endl;
  //	TRACE_ERR( "\tlanguage model cost "); // <<m_score[ScoreType::LanguageModelScore]<<endl;
  //	TRACE_ERR( "\tword penalty "); // <<(m_score[ScoreType::WordPenalty]*weightWordPenalty)<<endl;
	TRACE_ERR( "\tscore "<<m_totalScore - m_futureScore<<" + future cost "<<m_futureScore<<" = "<<m_totalScore<<endl);
  TRACE_ERR(  "\tunweighted feature scores: " << m_scoreBreakdown << endl);
	//PrintLMScores();
}

void Hypothesis::CleanupArcList()
{
	// point this hypo's main hypo to itself
	SetWinningHypo(this);

	if (!m_arcList) return;

	/* keep only number of arcs we need to create all n-best paths.
	 * However, may not be enough if only unique candidates are needed,
	 * so we'll keep all of arc list if nedd distinct n-best list
	 */
	const StaticData &staticData = StaticData::Instance();
	size_t nBestSize = staticData.GetNBestSize();
	bool distinctNBest = staticData.GetDistinctNBest();

	if (!distinctNBest && m_arcList->size() > nBestSize * 5)
	{ // prune arc list only if there too many arcs
		nth_element(m_arcList->begin()
							, m_arcList->begin() + nBestSize - 1
							, m_arcList->end()
							, CompareHypothesisTotalScore());

		// delete bad ones
		ArcList::iterator iter;
		for (iter = m_arcList->begin() + nBestSize ; iter != m_arcList->end() ; ++iter)
		{
			Hypothesis *arc = *iter;
			FREEHYPO(arc);
		}
		m_arcList->erase(m_arcList->begin() + nBestSize
										, m_arcList->end());
	}
	
	// set all remaining arc's main hypo variable to this hypo
	ArcList::iterator iter = m_arcList->begin();
	for (; iter != m_arcList->end() ; ++iter)
	{
		Hypothesis *arc = *iter;
		arc->SetWinningHypo(this);
	}
}

bool Hypothesis::IsCompletable() const
{
	// if all source words for this decode step is covered, then target sentence length must equal
	// to previous length. 
	const DecodeStep &decodeStep = StaticData::Instance().GetDecodeStep(m_decodeStepId);
	if (m_sourceCompleted.IsComplete(decodeStep))
	{
		return (m_targetSize[m_decodeStepId] == m_targetPhrase.GetSize());
	}
	
	// not all source words have been translated
	// create target bitmap. Hack - only values for this decode step are set
	WordsBitmap targetCompleted(GetSize());
	targetCompleted.SetValue(m_decodeStepId, 0, m_currTargetWordsRange.GetEndPos(), true);

	// call IsCompletable() in alignment obj
	return m_alignPair.IsCompletable(m_decodeStepId, m_sourceCompleted, targetCompleted);
}

TO_STRING_BODY(Hypothesis)
 
// friend
ostream& operator<<(ostream& out, const Hypothesis& hypothesis)
{	
	out << hypothesis.GetTargetPhrase();
	// words bitmap
	out << "[" << hypothesis.m_sourceCompleted << "] ";
	
	// scores
	out << " [total=" << hypothesis.GetTotalScore() << "]";
	out << " " << hypothesis.GetScoreBreakdown();

	// alignment
	out << hypothesis.GetAlignmentPair();
	return out;
}


std::string Hypothesis::GetSourcePhraseStringRep(const vector<FactorType> factorsToPrint) const 
{
	if (!m_prevHypo) { return ""; }
	if(m_sourcePhrase) 
	{
		return m_sourcePhrase->GetSubString(GetCurrSourceWordsRange()).GetStringRep(factorsToPrint);
	}
	else
	{ 
		return m_sourceInput.GetSubString(GetCurrSourceWordsRange()).GetStringRep(factorsToPrint);
	}	
}
std::string Hypothesis::GetTargetPhraseStringRep(const vector<FactorType> factorsToPrint) const 
{
	if (!m_prevHypo) { return ""; }
	return m_currTargetPhrase.GetStringRep(factorsToPrint);
}

std::string Hypothesis::GetSourcePhraseStringRep() const 
{
	vector<FactorType> allFactors;
	const size_t maxSourceFactors = StaticData::Instance().GetMaxNumFactors(Input);
	for(size_t i=0; i < maxSourceFactors; i++)
	{
		allFactors.push_back(i);
	}
	return GetSourcePhraseStringRep(allFactors);		
}
std::string Hypothesis::GetTargetPhraseStringRep() const 
{
	vector<FactorType> allFactors;
	const size_t maxTargetFactors = StaticData::Instance().GetMaxNumFactors(Output);
	for(size_t i=0; i < maxTargetFactors; i++)
	{
		allFactors.push_back(i);
	}
	return GetTargetPhraseStringRep(allFactors);
}
