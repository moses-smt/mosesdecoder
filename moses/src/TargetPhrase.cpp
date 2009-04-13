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
#include <algorithm>
#include "TargetPhrase.h"
#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "LanguageModel.h"
#include "StaticData.h"
#include "ScoreIndexManager.h"
#include "LMList.h"
#include "ScoreComponentCollection.h"
#include "Util.h"

using namespace std;

TargetPhrase::TargetPhrase(FactorDirection direction)
	//:Phrase(direction), m_ngramScore(0.0), m_fullScore(0.0), m_sourcePhrase(0)
	:Phrase(direction),m_transScore(0.0), m_ngramScore(0.0), m_fullScore(0.0), m_sourcePhrase(0)
	,m_subRangeCount(0)
	,m_trainingCount(1)
{
}

void TargetPhrase::SetScore()
{ // used when creating translations of unknown words:
	m_transScore = m_ngramScore = 0;	
	//m_ngramScore = 0;	
	m_fullScore = - StaticData::Instance().GetWeightWordPenalty();	
}

void TargetPhrase::SetScore(float score) 
{
	//we use an existing score producer to figure out information for score setting (number of scores and weights)
	//TODO: is this a good idea?
	ScoreProducer* prod = StaticData::Instance().GetPhraseDictionaries()[0];
	
	//get the weight list
	unsigned int id = prod->GetScoreBookkeepingID();
	
	const vector<float> &allWeights = StaticData::Instance().GetAllWeights();

	size_t beginIndex = StaticData::Instance().GetScoreIndexManager().GetBeginIndex(id);
	size_t endIndex = StaticData::Instance().GetScoreIndexManager().GetEndIndex(id);

	vector<float> weights;

	std::copy(allWeights.begin() +beginIndex, allWeights.begin() + endIndex,std::back_inserter(weights));
	
	//find out how many items are in the score vector for this producer	
	size_t numScores = prod->GetNumScoreComponents();

	//divide up the score among all of the score vectors
	vector <float> scoreVector(numScores,score/numScores);
	
	//Now we have what we need to call the full SetScore method
	SetScore(prod,scoreVector,weights,StaticData::Instance().GetWeightWordPenalty(),StaticData::Instance().GetAllLM());

}

void TargetPhrase::SetScore(const ScoreProducer* translationScoreProducer,
														const vector<float> &scoreVector, const vector<float> &weightT,
														float weightWP, const LMList &languageModels)
{
	assert(weightT.size() == scoreVector.size());
	// calc average score if non-best

	m_transScore = std::inner_product(scoreVector.begin(), scoreVector.end(), weightT.begin(), 0.0f);
	m_scoreBreakdown.PlusEquals(translationScoreProducer, scoreVector);
	
  // Replicated from TranslationOptions.cpp
	float totalNgramScore  = 0;
	float totalFullScore   = 0;

	LMList::const_iterator lmIter;
	for (lmIter = languageModels.begin(); lmIter != languageModels.end(); ++lmIter)
	{
		const LanguageModel &lm = **lmIter;
		
		if (lm.Useable(*this))
		{ // contains factors used by this LM
			const float weightLM = lm.GetWeight();
			float fullScore, nGramScore;

			lm.CalcScore(*this, fullScore, nGramScore);
			m_scoreBreakdown.Assign(&lm, nGramScore);

			// total LM score so far
			totalNgramScore  += nGramScore * weightLM;
			totalFullScore   += fullScore * weightLM;
			
		}
	}
  m_ngramScore = totalNgramScore;

	m_fullScore = m_transScore + totalFullScore
							- (this->GetSize() * weightWP);	 // word penalty
}

void TargetPhrase::RecalcLMScore(float weightWP, const LMList &languageModels)
{
  // Replicated from TranslationOptions.cpp
	float totalNgramScore  = 0;
	float totalFullScore   = 0;

	LMList::const_iterator lmIter;
	for (lmIter = languageModels.begin(); lmIter != languageModels.end(); ++lmIter)
	{
		const LanguageModel &lm = **lmIter;
		
		if (lm.Useable(*this))
		{ // contains factors used by this LM
			const float weightLM = lm.GetWeight();
			float fullScore, nGramScore;

			lm.CalcScore(*this, fullScore, nGramScore);
			m_scoreBreakdown.Assign(&lm, nGramScore);

			// total LM score so far
			totalNgramScore  += nGramScore * weightLM;
			totalFullScore   += fullScore * weightLM;
			
		}
	}

  m_ngramScore = totalNgramScore;
	m_fullScore = m_transScore + totalFullScore
							- (this->GetSize() * weightWP);	 // word penalty

}

void TargetPhrase::SetWeights(const ScoreProducer* translationScoreProducer, const vector<float> &weightT)
{
	// calling this function in case of confusion net input is undefined
	assert(StaticData::Instance().GetInputType()==SentenceInput); 
	
	/* one way to fix this, you have to make sure the weightT contains (in 
     addition to the usual phrase translation scaling factors) the input 
     weight factor as last element
	*/

	m_transScore = m_scoreBreakdown.PartialInnerProduct(translationScoreProducer, weightT);
}

void TargetPhrase::ResetScore()
{
	m_fullScore = m_ngramScore = 0;
	m_scoreBreakdown.ZeroAll();
}

TargetPhrase *TargetPhrase::MergeNext(const TargetPhrase &inputPhrase) const
{
	if (! IsCompatible(inputPhrase))
	{
		return NULL;
	}

	// ok, merge
	TargetPhrase *clone				= new TargetPhrase(*this);
	clone->m_sourcePhrase = m_sourcePhrase;
	int currWord = 0;
	const size_t len = GetSize();
	for (size_t currPos = 0 ; currPos < len ; currPos++)
	{
		const Word &inputWord	= inputPhrase.GetWord(currPos);
		Word &cloneWord = clone->GetWord(currPos);
		cloneWord.Merge(inputWord);
		
		currWord++;
	}

	return clone;
}

void TargetPhrase::Append(const WordsRange &sourceRange, const TargetPhrase &appendPhrase)
{
	//assert(sourceRange.GetNumWordsCovered() == appendPhrase.GetSourcePhrase()->GetSize());

	// alignment
	size_t sourceSize = m_alignmentPair.GetAlignmentPhrase(Input).GetSize();
	size_t endSourceSize = appendPhrase.GetAlignmentPair().GetAlignmentPhrase(Input).GetSize();

	WordsRange endTargetRange(GetSize(), GetSize() + appendPhrase.GetSize() - 1);

	m_alignmentPair.Append(appendPhrase.GetAlignmentPair(), sourceRange, endTargetRange);

	// words
	Phrase::Append(appendPhrase);

	// scores
	m_scoreBreakdown.PlusEquals(appendPhrase.m_scoreBreakdown);
	m_transScore	+= appendPhrase.m_transScore;
	
	// these scores are wrong, have to reset them after appending all subphrases
	//m_ngramScore
	//m_fullScore	

	m_subRangeCount++;
}

// helper functions
void AddAlignmentElement(AlignmentPhraseInserter &inserter
												 , const string &str
												 , size_t phraseSize
												 , size_t otherPhraseSize
												 , list<size_t> &uniformAlignment)
{
	// input
	vector<string> alignPhraseVector;
	alignPhraseVector = Tokenize(str);
	// "(0) (3) (1,2)"
	//		to
	// "(0)" "(3)" "(1,2)"
	assert (alignPhraseVector.size() == phraseSize) ;

	const size_t inputSize = alignPhraseVector.size();
	for (size_t pos = 0 ; pos < inputSize ; ++pos)
	{
		string alignElementStr = alignPhraseVector[pos];
		alignElementStr = alignElementStr.substr(1, alignElementStr.size() - 2);
		AlignmentElement *alignElement = new AlignmentElement(Tokenize<size_t>(alignElementStr, ","));
		// "(1,2)"
		//  to
		// [1] [2]
		if (alignElement->GetSize() == 0)
		{ // no alignment info. add uniform alignment, ie. can be aligned to any word
			alignElement->SetUniformAlignment(otherPhraseSize);
			uniformAlignment.push_back(pos);
		}

		**inserter = alignElement;
		(*inserter)++;		
	}
}

void TargetPhrase::CreateAlignmentInfo(const string &sourceStr
																			 , const string &targetStr
																			 , size_t sourceSize)
{
	AlignmentPhraseInserter sourceInserter = m_alignmentPair.GetInserter(Input)
													,targetInserter = m_alignmentPair.GetInserter(Output);
	list<size_t> uniformAlignmentSource
							,uniformAlignmentTarget;
	AddAlignmentElement(sourceInserter
										, sourceStr
										, sourceSize
										, GetSize()
										, uniformAlignmentSource);
	AddAlignmentElement(targetInserter
										, targetStr
										, GetSize()
										, sourceSize
										, uniformAlignmentTarget);
	// propergate uniform alignments to other side
	m_alignmentPair.GetAlignmentPhrase(Output).AddUniformAlignmentElement(uniformAlignmentSource);
	m_alignmentPair.GetAlignmentPhrase(Input).AddUniformAlignmentElement(uniformAlignmentTarget);
}

bool TargetPhrase::IsCompatible(const TargetPhrase &inputPhrase
																, size_t startPos
																, size_t endPos
																, bool allowSourceNullAlign) const
{
	// size has to be the same or less
	const size_t size = GetSize()
							,checkSize = endPos - startPos + 1;
	assert(checkSize >= inputPhrase.GetSize());

	if (checkSize > size)
		return false;

	const size_t maxNumFactors = StaticData::Instance().GetMaxNumFactors(this->GetDirection());
	size_t inputPos = 0;
	for (size_t currPos = startPos ; currPos <= endPos ; currPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < maxNumFactors ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(currPos, factorType)
									,*inputFactor	= inputPhrase.GetFactor(inputPos, factorType);
			if (thisFactor != NULL && inputFactor != NULL && thisFactor != inputFactor)
				return false;
		}
		inputPos++;
	}

	// alignment has to be consistent
	return inputPhrase.GetAlignmentPair().IsCompatible(GetAlignmentPair()
																										, 0, 0
																										, allowSourceNullAlign);
}

bool TargetPhrase::IsCompatible(const TargetPhrase &inputPhrase) const
{
	// size has to be the same
	const size_t size = GetSize();
	if (inputPhrase.GetSize() != size)
		return false;

	const size_t maxNumFactors = StaticData::Instance().GetMaxNumFactors(this->GetDirection());
	for (size_t currPos = 0 ; currPos < size ; currPos++)
	{
		for (unsigned int currFactor = 0 ; currFactor < maxNumFactors ; currFactor++)
		{
			FactorType factorType = static_cast<FactorType>(currFactor);
			const Factor *thisFactor 		= GetFactor(currPos, factorType)
									,*inputFactor	= inputPhrase.GetFactor(currPos, factorType);
			if (thisFactor != NULL && inputFactor != NULL && thisFactor != inputFactor)
				return false;
		}
	}

	// alignment has to be consistent
	return inputPhrase.GetAlignmentPair().IsCompatible(GetAlignmentPair(), 0, size);
}

bool TargetPhrase::IsCompatible(const TargetPhrase &inputPhrase, const std::vector<FactorType>& factorVec) const
{
  const Phrase &phrase = static_cast<const Phrase&>(inputPhrase);
  
	bool ret = IsCompatible(phrase, factorVec);
  
	// don't compare alignments if input phrase taken directly from phrase table
	return ret && (inputPhrase.GetSubRangeCount() <= 1 
								|| inputPhrase.GetAlignmentPair().IsCompatible(GetAlignmentPair(), 0, 0)
								);
}

bool TargetPhrase::IsCompatible(const Phrase &inputPhrase, const std::vector<FactorType>& factorVec) const
{
	const size_t size = GetSize();
	if (inputPhrase.GetSize() != size)	
		return false;

	for (size_t currPos = 0 ; currPos < size ; currPos++)
	{
		for (std::vector<FactorType>::const_iterator i = factorVec.begin();
		     i != factorVec.end(); ++i)
		{
			if (GetFactor(currPos, *i) != inputPhrase.GetFactor(currPos, *i))
				return false;
		}
	}
	return true;
}

void TargetPhrase::SetAlignment(const std::vector<std::string> &alignStrVec, FactorDirection direction)
{
	assert(direction == Output);
	FactorDirection otherDirection = (direction==Input)?Output:Input;

	AlignmentPhrase &alignmentPhrase= m_alignmentPair.GetAlignmentPhrase(direction)
		,&otherAlignmentPhrase	= m_alignmentPair.GetAlignmentPhrase(otherDirection);

	string alignStr, otherAlignStr;
	std::vector<std::string> otherAlignStrVec(m_sourcePhrase->GetSize(), "(");

	// create the other side
	for (size_t pos = 0 ; pos < alignStrVec.size() ; ++pos)
	{
		alignStr += alignStrVec[pos] + " ";

		string alignStrElement = alignStrVec[pos].substr(1, alignStrVec[pos].size() -2);
		vector<size_t> alignVec = Tokenize<size_t>(alignStrElement, ",");

		for (size_t index = 0 ; index < alignVec.size() ; ++index)
		{
			otherAlignStrVec[alignVec[index]] += SPrint(pos) + ",";
		}
	}

	// chop of end , & cap with )
	for (size_t pos = 0 ; pos < otherAlignStrVec.size() ; ++pos)
	{
		string &str = otherAlignStrVec[pos];
		otherAlignStr += str.substr(0, str.size() - 1) + ") ";
	}

	CreateAlignmentInfo(otherAlignStr, alignStr, m_sourcePhrase->GetSize());
	assert(alignStrVec.size() == alignmentPhrase.GetSize());

}

void TargetPhrase::SetTrainingCounts(size_t trainingCount, size_t sumTargetCount, size_t sumSourceCount)
{ 
	m_trainingCount		= trainingCount;
	m_sumTargetCount	= sumTargetCount;
	m_sumSourceCount	= sumSourceCount;
}

TO_STRING_BODY(TargetPhrase);

std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
	if (tp.m_sourcePhrase)
		os << *tp.m_sourcePhrase << " TO ";
			
  os	<< static_cast<const Phrase&>(tp) 
			<< ", " << tp.GetAlignmentPair()
			<< ", pC=" << tp.m_transScore << ", c=" << tp.m_fullScore
			<< ", " << tp.m_scoreBreakdown;
	return os;
}
