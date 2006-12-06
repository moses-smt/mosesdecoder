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
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "DummyScoreProducers.h"
#include "Hypothesis.h"
#include "Util.h"
#include "SquareMatrix.h"
#include "LexicalReordering.h"
#include "StaticData.h"
#include "InputType.h"
#include "LMList.h"
#include "hash.h"

using namespace std;

unsigned int Hypothesis::s_HypothesesCreated = 0;
ObjectPool<Hypothesis> Hypothesis::s_objectPool("Hypothesis", 300000);

Hypothesis::Hypothesis(InputType const& source, const TargetPhrase &emptyTarget)
	: m_prevHypo(NULL)
	, m_targetPhrase(emptyTarget)
	, m_sourcePhrase(0)
	, m_sourceCompleted(source.GetSize())
	, m_sourceInput(source)
	, m_currSourceWordsRange(NULL, NOT_FOUND, NOT_FOUND)
	, m_currTargetWordsRange(NULL, NOT_FOUND, NOT_FOUND)
	, m_wordDeleted(false)
	, m_languageModelStates(StaticData::Instance()->GetLMSize(), LanguageModelSingleFactor::UnknownState)
	, m_arcList(NULL)
	, m_id(0)
	, m_lmstats(NULL)
{	// used for initial seeding of trans process	
	// initialize scores
	//_hash_computed = false;
	s_HypothesesCreated = 1;
	ResetScore();	
}

/***
 * continue prevHypo by appending the phrases in transOpt
 */
Hypothesis::Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt)
	: m_prevHypo(&prevHypo)
	, m_targetPhrase(transOpt.GetTargetPhrase())
	, m_sourcePhrase(0)
	, m_sourceCompleted				(prevHypo.m_sourceCompleted )
	, m_sourceInput						(prevHypo.m_sourceInput)
	, m_currSourceWordsRange	(transOpt.GetSourceWordsRange())
	, m_currTargetWordsRange	(transOpt.GetDecodeStep(), prevHypo.m_currTargetWordsRange.GetEndPos() + 1
														 ,prevHypo.m_currTargetWordsRange.GetEndPos() + transOpt.GetTargetPhrase().GetSize())
	, m_wordDeleted(false)
	,	m_totalScore(0.0f)
	,	m_futureScore(0.0f)
	, m_scoreBreakdown				(prevHypo.m_scoreBreakdown)
	, m_languageModelStates(prevHypo.m_languageModelStates)
	, m_arcList(NULL)
	, m_id(s_HypothesesCreated++)
	, m_lmstats(NULL)
{
	const DecodeStep &decodeStep = transOpt.GetDecodeStep();

	// assert that we are not extending our hypothesis by retranslating something
	// that this hypothesis has already translated!
	assert(!m_sourceCompleted.Overlap(m_currSourceWordsRange));	

	//_hash_computed = false;
  m_sourceCompleted.SetValue(m_currSourceWordsRange.GetStartPos(), m_currSourceWordsRange.GetEndPos(), true);
  m_wordDeleted = transOpt.IsDeletionOption();
	m_scoreBreakdown.PlusEquals(transOpt.GetScoreBreakdown());
}

Hypothesis::~Hypothesis()
{
	if (m_arcList) 
	{
		ArcList::iterator iter;
		for (iter = m_arcList->begin() ; iter != m_arcList->end() ; ++iter)
		{
			s_objectPool.freeObject (*iter);
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
	Hypothesis *ptr = s_objectPool.getPtr();
	return new(ptr) Hypothesis(prevHypo, transOpt);
}
/***
 * return the subclass of Hypothesis most appropriate to the given target phrase
 */

Hypothesis* Hypothesis::Create(InputType const& m_source, const TargetPhrase &emptyTarget)
{
	Hypothesis *ptr = s_objectPool.getPtr();
	return new(ptr) Hypothesis(m_source, emptyTarget);
}

//void Hypothesis::GenerateNGramCompareHash() const
//{
//	_hash = quick_hash((const char*)&m_languageModelStates[0], sizeof(LanguageModelSingleFactor::State) * m_languageModelStates.size(), 0xcafe5137);
//	_hash_computed = true;
//	vector<size_t> wordCoverage = m_sourceCompleted.GetCompressedRepresentation();
//	_hash = quick_hash((const char*)&wordCoverage[0], sizeof(size_t)*wordCoverage.size(), _hash);
//}

/** check, if two hypothesis can be recombined.
    this is actually a sorting function that allows us to
    keep an ordered list of hypotheses. This makes recombination
    much quicker. 
*/
int Hypothesis::NGramCompare(const Hypothesis &compare) const
{ // -1 = this < compare
	// +1 = this > compare
	// 0	= this ==compare
	if (m_languageModelStates < compare.m_languageModelStates) return -1;
	if (m_languageModelStates > compare.m_languageModelStates) return 1;
	if (m_sourceCompleted.GetCompressedRepresentation() < compare.m_sourceCompleted.GetCompressedRepresentation()) return -1;
	if (m_sourceCompleted.GetCompressedRepresentation() > compare.m_sourceCompleted.GetCompressedRepresentation()) return 1;
	if (m_currSourceWordsRange.GetEndPos() < compare.m_currSourceWordsRange.GetEndPos()) return -1;
	if (m_currSourceWordsRange.GetEndPos() > compare.m_currSourceWordsRange.GetEndPos()) return 1;
	if (! StaticData::Instance()->GetSourceStartPosMattersForRecombination()) return 0;
	if (m_currSourceWordsRange.GetStartPos() < compare.m_currSourceWordsRange.GetStartPos()) return -1;
	if (m_currSourceWordsRange.GetStartPos() > compare.m_currSourceWordsRange.GetStartPos()) return 1;
	return 0;
}

/** Calculates the overall language model score by combining the scores
 * of language models generated for each of the factors.  Because the factors
 * represent a variety of tag sets, and because factors with smaller tag sets 
 * (such as POS instead of words) allow us to calculate richer statistics, we
 * allow a different length of n-gram to be specified for each factor.
 * /param lmListInitial todo - describe this parameter 
 * /param lmListEnd todo - describe this parameter
 */
void Hypothesis::CalcLMScore(const LMList &languageModels)
{
	const size_t startPos	= m_currTargetWordsRange.GetStartPos();
	LMList::const_iterator iterLM;

	// will be null if LM stats collection is disabled
	if (StaticData::Instance()->IsComputeLMBackoffStats()) {
		m_lmstats = new vector<vector<unsigned int> >(languageModels.size(), vector<unsigned int>(0));
	}

	size_t lmIdx = 0;

	// already have LM scores from previous and trigram score of poss trans.
	// just need trigram score of the words of the start of current phrase	
	for (iterLM = languageModels.begin() ; iterLM != languageModels.end() ; ++iterLM,++lmIdx)
	{
		const LanguageModel &languageModel = **iterLM;
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
			//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;

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
			if (m_sourceCompleted.IsComplete())
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
				if (m_lmstats) {
					(*m_lmstats)[lmIdx].resize((*m_lmstats)[lmIdx].size() + 1); // extra space for the last call
					lmScore += languageModel.GetValue(contextFactor, &m_languageModelStates[lmIdx], &(*m_lmstats)[lmIdx][nLmCallCount++]);
				} else
					lmScore	+= languageModel.GetValue(contextFactor, &m_languageModelStates[lmIdx]);
			} else {
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
	}
}

void Hypothesis::CalcDistortionScore()

{
	const DistortionScoreProducer *dsp = StaticData::Instance()->GetDistortionScoreProducer();
	float distortionScore = dsp->CalculateDistortionScore(
			m_prevHypo->GetCurrSourceWordsRange(),
			this->GetCurrSourceWordsRange()
     );
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
void Hypothesis::CalcScore(const StaticData& staticData, const SquareMatrix &futureScore) 
{
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

void Hypothesis::CalcFutureScore(const SquareMatrix &futureScore)
{
	const size_t maxSize= numeric_limits<size_t>::max();
	size_t	start				= maxSize;
	m_futureScore	= 0.0f;
	for(size_t currPos = 0 ; currPos < m_sourceCompleted.GetSize() ; currPos++) 
	{
		if(m_sourceCompleted.GetValue(currPos) == 0 && start == maxSize)
		{
			start = currPos;
		}
		if(m_sourceCompleted.GetValue(currPos) == 1 && start != maxSize) 
		{
//			m_score[ScoreType::FutureScoreEnum] += futureScore[start][currPos - 1];
			m_futureScore += futureScore.GetScore(start, currPos - 1);
			start = maxSize;
		}
	}
	if (start != maxSize)
	{
//		m_score[ScoreType::FutureScoreEnum] += futureScore[start][m_sourceCompleted.GetSize() - 1];
		m_futureScore += futureScore.GetScore(start, m_sourceCompleted.GetSize() - 1);
	}

	// add future costs for distortion model
	if(StaticData::Instance()->UseDistortionFutureCosts())
		m_futureScore += m_sourceCompleted.GetFutureCosts(m_currSourceWordsRange.GetEndPos()) * StaticData::Instance()->GetWeightDistortion();
	
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
  int end = (int)(m_prevHypo->m_targetPhrase.GetSize()-1);
  int start = end-1;
  if ( start < 0 ) start = 0;
  if ( m_prevHypo->m_currTargetWordsRange.GetStartPos() == NOT_FOUND ) {
    TRACE_ERR( "<s> ");
  }
  else {
    TRACE_ERR( "... ");
  }
  if (end>=0) {
    WordsRange range(start, end);
    TRACE_ERR( m_prevHypo->m_targetPhrase.GetSubString(range) << " ");
  }
  TRACE_ERR( ")"<<endl);
	TRACE_ERR( "\tbase score "<< (m_prevHypo->m_totalScore - m_prevHypo->m_futureScore) <<endl);
	TRACE_ERR( "\tcovering "<<m_currSourceWordsRange.GetStartPos()<<"-"<<m_currSourceWordsRange.GetEndPos()<<": "<< source.GetSubString(m_currSourceWordsRange)  <<endl);
	TRACE_ERR( "\ttranslated as: "<<m_targetPhrase<<endl); // <<" => translation cost "<<m_score[ScoreType::PhraseTrans];
	if (m_wordDeleted) TRACE_ERR( "\tword deleted"<<endl); 
  //	TRACE_ERR( "\tdistance: "<<GetCurrSourceWordsRange().CalcDistortion(m_prevHypo->GetCurrSourceWordsRange())); // << " => distortion cost "<<(m_score[ScoreType::Distortion]*weightDistortion)<<endl;
  //	TRACE_ERR( "\tlanguage model cost "); // <<m_score[ScoreType::LanguageModelScore]<<endl;
  //	TRACE_ERR( "\tword penalty "); // <<(m_score[ScoreType::WordPenalty]*weightWordPenalty)<<endl;
	TRACE_ERR( "\tscore "<<m_totalScore - m_futureScore<<" + future cost "<<m_futureScore<<" = "<<m_totalScore<<endl);
  TRACE_ERR(  "\tunweighted feature scores: " << m_scoreBreakdown << endl);
	//PrintLMScores();
}

void Hypothesis::InitializeArcs()
{
	// point this hypo's main hypo to itself
	SetWinningHypo(this);

	if (!m_arcList) return;

	// set all arc's main hypo variable to this hypo
	ArcList::iterator iter = m_arcList->begin();
	for (; iter != m_arcList->end() ; ++iter)
	{
		Hypothesis *arc = *iter;
		arc->SetWinningHypo(this);
	}
}

TO_STRING_BODY(Hypothesis)
 
// friend
ostream& operator<<(ostream& out, const Hypothesis& hypothesis)
{	
	hypothesis.ToStream(out);
	// words bitmap
	out << "[" << hypothesis.m_sourceCompleted << "] ";
	
	// scores
	out << " [total=" << hypothesis.GetTotalScore() << "]";
	out << " " << hypothesis.GetScoreBreakdown();
	return out;
}


std::string Hypothesis::GetSourcePhraseStringRep(const vector<FactorType> factorsToPrint) const 
{
	if (!m_prevHypo) { return ""; }
	if(m_sourcePhrase) 
	{
		return m_sourcePhrase->GetSubString(m_currSourceWordsRange).GetStringRep(factorsToPrint);
	}
	else
	{ 
		return m_sourceInput.GetSubString(m_currSourceWordsRange).GetStringRep(factorsToPrint);
	}	
}
std::string Hypothesis::GetTargetPhraseStringRep(const vector<FactorType> factorsToPrint) const 
{
	if (!m_prevHypo) { return ""; }
	return m_targetPhrase.GetStringRep(factorsToPrint);
}

std::string Hypothesis::GetSourcePhraseStringRep() const 
{
	vector<FactorType> allFactors;
	const size_t maxSourceFactors = StaticData::Instance()->GetMaxNumFactors(Input);
	for(size_t i=0; i < maxSourceFactors; i++)
	{
		allFactors.push_back(i);
	}
	return GetSourcePhraseStringRep(allFactors);		
}
std::string Hypothesis::GetTargetPhraseStringRep() const 
{
	vector<FactorType> allFactors;
	const size_t maxTargetFactors = StaticData::Instance()->GetMaxNumFactors(Output);
	for(size_t i=0; i < maxTargetFactors; i++)
	{
		allFactors.push_back(i);
	}
	return GetTargetPhraseStringRep(allFactors);
}
