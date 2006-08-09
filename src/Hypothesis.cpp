// $Id$

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
#include "Input.h"
#include "LMList.h"
#include "hash.h"

using namespace std;

unsigned int Hypothesis::s_numNodes = 0;
unsigned int Hypothesis::s_HypothesesCreated = 0;
ObjectPool<Hypothesis> Hypothesis::s_objectPool("Hypothesis", 300000);

Hypothesis::Hypothesis(InputType const& source, const TargetPhrase &emptyTarget)
	: m_prevHypo(NULL)
	, m_targetPhrase(emptyTarget)
	, m_sourcePhrase(0)
	, m_sourceCompleted(source.GetSize())
	, m_sourceInput(source)
	, m_currSourceWordsRange(NOT_FOUND, NOT_FOUND)
	, m_currTargetWordsRange(NOT_FOUND, NOT_FOUND)
	, m_wordDeleted(false)
	, m_languageModelStates(StaticData::Instance()->GetLMSize(), LanguageModelSingleFactor::UnknownState)
	, m_arcList(NULL)
	, m_id(s_HypothesesCreated++)
{	// used for initial seeding of trans process	
	// initialize scores
	_hash_computed = false;
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
	, m_currTargetWordsRange	( prevHypo.m_currTargetWordsRange.GetEndPos() + 1
														 ,prevHypo.m_currTargetWordsRange.GetEndPos() + transOpt.GetTargetPhrase().GetSize())
	, m_wordDeleted(false)
	,	m_totalScore(0.0f)
	,	m_futureScore(0.0f)
	, m_scoreBreakdown				(prevHypo.m_scoreBreakdown)
	, m_languageModelStates(prevHypo.m_languageModelStates)
	, m_arcList(NULL)
	, m_id(s_HypothesesCreated++)
{
	// assert that we are not extending our hypothesis by retranslating something
	// that this hypothesis has already translated!
	assert(!m_sourceCompleted.Overlap(m_currSourceWordsRange));	

	_hash_computed = false;
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
		m_arcList = 0;
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

bool Hypothesis::IsCompatible(const Phrase &phrase) const
{
	// make sure factors don't contradict each other
	// similar to phrase comparison

	if (m_currTargetWordsRange.GetWordsCount() != phrase.GetSize())
	{
		return false;
	}
	size_t hypoSize = GetSize();

	size_t transOptPos = 0;
	for (size_t hypoPos = hypoSize - m_currTargetWordsRange.GetWordsCount() ; hypoPos < hypoSize ; hypoPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(hypoPos, factorType)
									,*compareFactor	= phrase.GetFactor(transOptPos, factorType);
			if (thisFactor != NULL && compareFactor != NULL && thisFactor != compareFactor)
				return false;
		}
		transOptPos++;
	}
	return true;
}

#if 0
void Hypothesis::GenerateNGramCompareKey(size_t contextSize)
{
  struct MD5Context md5c;

  MD5Init(&md5c);
  size_t thisSize = this->GetSize();
  size_t effectiveContextSize = std::min(thisSize, contextSize);
  int start = thisSize - effectiveContextSize;

  if (m_currTargetWordsRange.GetWordsCount() > 0) // initial hypothesis check
	{
	  const Hypothesis *curHyp = this;
	  int curStart = 0;
	  while (start < (curStart = curHyp->m_currTargetWordsRange.GetStartPos())) {
	    for (int col = curHyp->m_currTargetWordsRange.GetEndPos(); col >= curStart; col--) {
	      MD5Update(&md5c,
	        (unsigned char*)curHyp->GetCurrFactorArray(col - curStart),
	        sizeof(FactorArray));
				curHyp = curHyp->m_prevHypo;
	    }
	  }
	  for (int col = curHyp->m_currTargetWordsRange.GetEndPos(); col >= (int)start; col--) {
	    MD5Update(&md5c,
	      (unsigned char*)curHyp->GetCurrFactorArray(col - curStart),
	      sizeof(FactorArray));
	  }
	}
  MD5Final(m_compSignature, &md5c);
}
#endif

void Hypothesis::GenerateNGramCompareHash() const
{
	_hash = quick_hash((const char*)&m_languageModelStates[0], sizeof(LanguageModelSingleFactor::State) * m_languageModelStates.size(), 0xcafe5137);
	_hash_computed = true;
	vector<size_t> wordCoverage = m_sourceCompleted.GetCompressedReprentation();
	_hash = quick_hash((const char*)&wordCoverage[0], sizeof(size_t)*wordCoverage.size(), _hash);
}

int Hypothesis::NGramCompare(const Hypothesis &compare) const
{ // -1 = this < compare
	// +1 = this > compare
	// 0	= this ==compare
	if (m_languageModelStates < compare.m_languageModelStates) return -1;
	if (m_languageModelStates > compare.m_languageModelStates) return 1;
	return 0;
}
/**
 * Calculates the overall language model score by combining the scores
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
	size_t lmIdx = 0;

	// already have LM scores from previous and trigram score of poss trans.
	// just need trigram score of the words of the start of current phrase	
	for (iterLM = languageModels.begin() ; iterLM != languageModels.end() ; ++iterLM,++lmIdx)
	{
		const LanguageModelSingleFactor &languageModel = **iterLM;
		size_t nGramOrder			= languageModel.GetNGramOrder();
		size_t currEndPos			= m_currTargetWordsRange.GetEndPos();
		float lmScore;

		if(m_currTargetWordsRange.GetWordsCount() == 0) {
			lmScore = 0; //the score associated with dropping source words is not part of the language model
		} else { //non-empty target phrase
			// 1st n-gram
			vector<FactorArrayWrapper> contextFactor(nGramOrder);
			size_t index = 0;
			for (int currPos = (int) startPos - (int) nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
			{
				if (currPos >= 0)
					contextFactor[index++] = GetFactorArray(currPos);
				else			
					contextFactor[index++] = languageModel.GetSentenceStartArray();
			}
			lmScore	= languageModel.GetValue(contextFactor);
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
				contextFactor.back() = GetFactorArray(currPos);

				lmScore	+= languageModel.GetValue(contextFactor);
				//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;		
			}

			// end of sentence
			if (m_sourceCompleted.IsComplete())
			{
				const size_t size = GetSize();
				contextFactor.back() = languageModel.GetSentenceEndArray();
	
				for (size_t i = 0 ; i < nGramOrder - 1 ; i ++)
				{
					int currPos = size - nGramOrder + i + 1;
					if (currPos < 0)
						contextFactor[i] = languageModel.GetSentenceStartArray();
					else
						contextFactor[i] = GetFactorArray((size_t)currPos);
				}
				lmScore	+= languageModel.GetValue(contextFactor, &m_languageModelStates[lmIdx]);
			} else {
				for (size_t currPos = endPos+1; currPos <= currEndPos; currPos++) {
					for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
						contextFactor[i] = contextFactor[i + 1];
					contextFactor.back() = GetFactorArray(currPos);
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
	m_scoreBreakdown.PlusEquals(staticData.GetWordPenaltyProducer(), - (float) m_currTargetWordsRange.GetWordsCount()); 

	// FUTURE COST
	CalcFutureScore(futureScore);

	
	//LEXICAL REORDERING COST
	std::vector<LexicalReordering*> m_reorderModels = staticData.GetReorderModels();
	for(int i = 0; i < m_reorderModels.size(); i++)
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


int Hypothesis::GetId()const{
	return m_id;
}

const Hypothesis* Hypothesis::GetPrevHypo()const{
	return m_prevHypo;
}

/**
 * print hypothesis information for pharaoh-style logging
 */
void Hypothesis::PrintHypothesis(const InputType &source, float /*weightDistortion*/, float /*weightWordPenalty*/) const
{
	// PLEASE DON'T WRITE TO COUT directly. use TRACE_ERR or cerr 
  TRACE_ERR("creating hypothesis "<< m_id <<" from "<< m_prevHypo->m_id<<" ( ");
  int end = m_prevHypo->m_targetPhrase.GetSize()-1;
  int start = end-1;
  if ( start < 0 ) start = 0;
  if ( m_prevHypo->m_currTargetWordsRange.GetStartPos() == NOT_FOUND ) {
    TRACE_ERR("<s> ");
  }
  else {
    TRACE_ERR("... ");
  }
  if (end>=0) {
    WordsRange range(start, end);
    TRACE_ERR(m_prevHypo->m_targetPhrase.GetSubString(range) << " ");
  }
  TRACE_ERR(")"<<endl);
	TRACE_ERR("\tbase score "<< (m_prevHypo->m_totalScore - m_prevHypo->m_futureScore) <<endl);
	TRACE_ERR("\tcovering "<<m_currSourceWordsRange.GetStartPos()<<"-"<<m_currSourceWordsRange.GetEndPos()<<": "<< source.GetSubString(m_currSourceWordsRange)  <<endl);
	TRACE_ERR("\ttranslated as: "<<m_targetPhrase); // <<" => translation cost "<<m_score[ScoreType::PhraseTrans];
  if (m_wordDeleted) TRACE_ERR("   word_deleted"); 
  TRACE_ERR(endl);
	TRACE_ERR("\tdistance: "<<GetCurrSourceWordsRange().CalcDistortion(m_prevHypo->GetCurrSourceWordsRange())); // << " => distortion cost "<<(m_score[ScoreType::Distortion]*weightDistortion)<<endl;
	TRACE_ERR("\tlanguage model cost "); // <<m_score[ScoreType::LanguageModelScore]<<endl;
	TRACE_ERR("\tword penalty "); // <<(m_score[ScoreType::WordPenalty]*weightWordPenalty)<<endl;
	TRACE_ERR("\tscore "<<m_totalScore - m_futureScore<<" + future cost "<<m_futureScore<<" = "<<m_totalScore<<endl);
  TRACE_ERR( "\tunweighted feature scores: " << m_scoreBreakdown << endl);
	//PrintLMScores();
	 
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


std::string Hypothesis::GetSourcePhraseStringRep() const 
{
	if (!m_prevHypo) { return ""; }
	if(m_sourcePhrase) {
		assert(m_sourcePhrase->ToString()==m_sourcePhrase->GetStringRep(WordsRange(0,m_sourcePhrase->GetSize()-1)));
		return m_sourcePhrase->ToString();
	}
	else 
		return m_sourceInput.GetStringRep(m_currSourceWordsRange);
		
}
std::string Hypothesis::GetTargetPhraseStringRep() const 
{
	if (!m_prevHypo) { return ""; }
	return m_targetPhrase.ToString();
}
