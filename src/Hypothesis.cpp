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
#include <assert.h>
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


Hypothesis::Hypothesis(InputType const& source)
	: m_prevHypo(NULL)
	, m_targetPhrase(Output)
	, m_sourceCompleted(source.GetSize())
	, m_sourceInput(source)
	, m_currSourceWordsRange(NOT_FOUND, NOT_FOUND)
	, m_currTargetWordsRange(NOT_FOUND, NOT_FOUND)
	, m_wordDeleted(false)
#ifdef N_BEST
	, m_arcList(NULL)
#endif
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
	, m_sourceCompleted				(prevHypo.m_sourceCompleted )
	, m_sourceInput						(prevHypo.m_sourceInput)
	, m_currSourceWordsRange	(transOpt.GetSourceWordsRange())
	, m_currTargetWordsRange	( prevHypo.m_currTargetWordsRange.GetEndPos() + 1
														 ,prevHypo.m_currTargetWordsRange.GetEndPos() + transOpt.GetPhrase().GetSize())
	,	m_totalScore(0.0f)
	,	m_futureScore(0.0f)
	, m_wordDeleted(false)
	, m_id(s_HypothesesCreated++)
	, m_scoreBreakdown				(prevHypo.m_scoreBreakdown)
#ifdef N_BEST
	, m_arcList(NULL)
#endif
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
#ifdef N_BEST
	if (m_arcList) {
		RemoveAllInColl< ArcList::iterator >(*m_arcList);
		delete m_arcList;
	}
#endif
}

#ifdef N_BEST
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
#endif

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
	return new Hypothesis(prevHypo, transOpt);
}
/***
 * return the subclass of Hypothesis most appropriate to the given target phrase
 */

Hypothesis* Hypothesis::Create(InputType const& m_source)
{
	return new Hypothesis(m_source);
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
	_hash = 0xcafe5137;						// random
	const size_t thisSize			= GetSize();

	for (size_t currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
	{
		size_t ngramMax = StaticData::Instance()->GetMaxNGramOrderForFactorId(currFactor);
		if (ngramMax < 2) continue;  // unigrams have no context

		const size_t minSize		= std::min(ngramMax-1, thisSize);
		_hash = quick_hash((const char*)&minSize, sizeof(size_t), _hash);

		for (size_t currNGram = 1 ; currNGram <= minSize ; currNGram++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(thisSize - currNGram, factorType);
			_hash = quick_hash((const char*)&thisFactor, sizeof(const Factor*), _hash);
		}
	}
	vector<size_t> wordCoverage = m_sourceCompleted.GetCompressedReprentation();
	_hash = quick_hash((const char*)&wordCoverage[0], sizeof(size_t)*wordCoverage.size(), _hash);
	_hash_computed = true;
}

int Hypothesis::NGramCompare(const Hypothesis &compare) const
{ // -1 = this < compare
	// +1 = this > compare
	// 0	= this ==compare

	const size_t thisSize			= GetSize();
	const size_t compareSize	= compare.GetSize();

	for (size_t currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
	{
		size_t ngramMax = StaticData::Instance()->GetMaxNGramOrderForFactorId(currFactor);
		if (ngramMax < 2) continue;  // unigrams have no context

		const size_t minSize		= std::min(ngramMax-1, thisSize)
					, minCompareSize	= std::min(ngramMax-1, compareSize);
		if ( minSize != minCompareSize )
		{ // quick decision
			return (minSize < minCompareSize) ? -1 : 1;
		}

		for (size_t currNGram = 1 ; currNGram <= minSize ; currNGram++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(thisSize - currNGram, factorType)
				,*compareFactor	= compare.GetFactor(compareSize - currNGram, factorType);
			// just use address of factor
			if (thisFactor < compareFactor)
				return -1;
			if (thisFactor > compareFactor)
				return 1;
		}
	}
	// identical
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

	// for LM which are not in PossTran
	// must go through each trigram in current phrase
	for (iterLM = languageModels.begin() ; iterLM != languageModels.end() ; ++iterLM)
	{
		const LanguageModel &languageModel = **iterLM;
		FactorType factorType	= languageModel.GetFactorType();
		size_t nGramOrder			= languageModel.GetNGramOrder();
		float lmScore;

		// 1st n-gram
		vector<const Factor*> contextFactor(nGramOrder);
		size_t index = 0;
		for (int currPos = (int) startPos - (int) nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
		{
			if (currPos >= 0)
				contextFactor[index++] = GetFactor(currPos, factorType);
			else			
				contextFactor[index++] = languageModel.GetSentenceStart();
		}		
		lmScore	= languageModel.GetValue(contextFactor);
		//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;

		// main loop
		for (size_t currPos = startPos + 1 ; currPos <= m_currTargetWordsRange.GetEndPos() ; currPos++)
		{
			// shift all args down 1 place
			for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];

			// add last factor
			contextFactor.back() = GetFactor(currPos, factorType);

			lmScore	+= languageModel.GetValue(contextFactor);
			//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;
		
		}

		// end of sentence
		if (m_sourceCompleted.IsComplete())
		{
			// shift all args down 1 place
			for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
				contextFactor[i] = contextFactor[i + 1];

			contextFactor.back() = languageModel.GetSentenceEnd();
			lmScore	+= languageModel.GetValue(contextFactor);
		}
		m_scoreBreakdown.PlusEquals(&languageModel, lmScore);
	}

	// for LM which are in possTran
	// already have LM scores from previous and trigram score of poss trans.
	// just need trigram score of the words of the start of current phrase	
	for (iterLM = languageModels.begin() ; iterLM != languageModels.end() ; ++iterLM)
	{
		const LanguageModel &languageModel = **iterLM;
		FactorType factorType = languageModel.GetFactorType();
		size_t nGramOrder			= languageModel.GetNGramOrder();
		float lmScore;

		if(m_currTargetWordsRange.GetWordsCount() > 0) //non-empty target phrase
		{
			// 1st n-gram
			vector<const Factor*> contextFactor(nGramOrder);
			size_t index = 0;
			for (int currPos = (int) startPos - (int) nGramOrder + 1 ; currPos <= (int) startPos ; currPos++)
			{
				if (currPos >= 0)
					contextFactor[index++] = GetFactor(currPos, factorType);
				else			
					contextFactor[index++] = languageModel.GetSentenceStart();
			}
			lmScore	= languageModel.GetValue(contextFactor);
			//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;

			// main loop
			size_t endPos = std::min(startPos + nGramOrder - 2
															, m_currTargetWordsRange.GetEndPos());
			for (size_t currPos = startPos + 1 ; currPos <= endPos ; currPos++)
			{
				// shift all args down 1 place
				for (size_t i = 0 ; i < nGramOrder - 1 ; i++)
					contextFactor[i] = contextFactor[i + 1];
	
				// add last factor
				contextFactor.back() = GetFactor(currPos, factorType);
	
				lmScore	+= languageModel.GetValue(contextFactor);
				//cout<<"context factor: "<<languageModel.GetValue(contextFactor)<<endl;		
			}

			// end of sentence
			if (m_sourceCompleted.IsComplete())
			{
				const size_t size = GetSize();
				contextFactor.back() = languageModel.GetSentenceEnd();
	
				for (size_t i = 0 ; i < nGramOrder - 1 ; i ++)
				{
					int currPos = size - nGramOrder + i + 1;
					if (currPos < 0)
						contextFactor[i] = languageModel.GetSentenceStart();
					else
						contextFactor[i] = GetFactor((size_t)currPos, factorType);
				}
				lmScore	+= languageModel.GetValue(contextFactor);
			}
		}
		else lmScore = 0; //the score associated with dropping source words is not part of the language model
		
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
	LexicalReordering *m_lexReorder = staticData.GetLexReorder();
	if (m_lexReorder) {
		std::vector<float> lroScores;
		lroScores.push_back(m_lexReorder->CalcScore(this,LexReorderType::Forward));
		lroScores.push_back(m_lexReorder->CalcScore(this,LexReorderType::Backward));
		m_scoreBreakdown.PlusEquals(m_lexReorder, lroScores);
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
	//	m_score[ScoreType::FutureScoreEnum] += m_sourceCompleted.GetFutureCosts(m_currSourceWordsRange.GetEndPos()) * StaticData::Instance()->GetWeightDistortion();
	
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
void Hypothesis::PrintHypothesis(const InputType &source, float weightDistortion, float weightWordPenalty) const{
  cout<<"creating hypothesis "<< m_id <<" from "<< m_prevHypo->m_id<<" ( ";
  int end = m_prevHypo->m_targetPhrase.GetSize()-1;
  int start = end-1;
  if ( start < 0 ) start = 0;
  if ( m_prevHypo->m_currTargetWordsRange.GetStartPos() == NOT_FOUND ) {
    cout << "<s> ";
  }
  else {
    cout << "... ";
  }
  if (end>=0) {
    WordsRange range(start, end);
    cout << m_prevHypo->m_targetPhrase.GetSubString(range) << " ";
  }
  cout<<")"<<endl;
	cout<<"\tbase score "<< (m_prevHypo->m_totalScore - m_prevHypo->m_futureScore) <<endl;
	cout<<"\tcovering "<<m_currSourceWordsRange.GetStartPos()<<"-"<<m_currSourceWordsRange.GetEndPos()<<": "<< source.GetSubString(m_currSourceWordsRange)  <<endl;
	cout<<"\ttranslated as: "<<m_targetPhrase; // <<" => translation cost "<<m_score[ScoreType::PhraseTrans];
  if (m_wordDeleted) cout <<"   word_deleted"; 
  cout<<endl;
	cout<<"\tdistance: "<<GetCurrSourceWordsRange().CalcDistortion(m_prevHypo->GetCurrSourceWordsRange()); // << " => distortion cost "<<(m_score[ScoreType::Distortion]*weightDistortion)<<endl;
	cout<<"\tlanguage model cost "; // <<m_score[ScoreType::LanguageModelScore]<<endl;
	cout<<"\tword penalty "; // <<(m_score[ScoreType::WordPenalty]*weightWordPenalty)<<endl;
	cout<<"\tscore "<<m_totalScore - m_futureScore<<" + future cost "<<m_futureScore<<" = "<<m_totalScore<<endl;
  cout << "\tunweighted feature scores: " << m_scoreBreakdown << endl;
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
	out << " [0.12, " << hypothesis.GetTotalScore() << "]";
	out << " " << hypothesis.GetScoreBreakdown();
	return out;
}
