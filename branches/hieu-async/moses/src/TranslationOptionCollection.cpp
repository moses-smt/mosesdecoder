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

#include <algorithm>
#include "TranslationOptionCollection.h"
#include "Sentence.h"
#include "DecodeStepTranslation.h"
#include "LanguageModel.h"
#include "PhraseDictionaryMemory.h"
#include "FactorCollection.h"
#include "InputType.h"
#include "Util.h"
#include "StaticData.h"

using namespace std;

/** constructor; since translation options are indexed by coverage span, the corresponding data structure is initialized here 
	* This fn should be called by inherited classes
*/
TranslationOptionCollection::TranslationOptionCollection(InputType const& src, size_t maxNoTransOptPerCoverage)
	: m_source(src)
	, m_maxNoTransOptPerCoverage(maxNoTransOptPerCoverage)
	, m_futureScore(src.GetSize())
{
}

/** destructor, clears out data structures */
TranslationOptionCollection::~TranslationOptionCollection()
{
	for (size_t decodeStepId = 0 ; decodeStepId < m_collection.size() ; ++decodeStepId)
	{
		// delete all trans opt
		size_t size = m_source.GetSize();
		for (size_t startPos = 0 ; startPos < size ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < size ; ++endPos)
			{
				RemoveAllInColl(GetTranslationOptionList(decodeStepId, startPos, endPos));
			}
		}
	}
}

/** helper for pruning */
bool CompareTranslationOption(const TranslationOption *a, const TranslationOption *b)
{
	return a->GetFutureScore() > b->GetFutureScore();
}

void TranslationOptionCollection::Prune()
{	
	size_t size = m_source.GetSize();
	
	// prune to max no. of trans opt
	if (m_maxNoTransOptPerCoverage == 0)
		return;

	for (size_t decodeStepId = 0 ; decodeStepId < m_collection.size() ; ++decodeStepId)
	{
		size_t total = 0;
		size_t totalPruned = 0;
		for (size_t startPos = 0 ; startPos < size ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < size ; ++endPos)
			{
				TranslationOptionList &fullList = GetTranslationOptionList(decodeStepId, startPos, endPos);
				total += fullList.size();
				if (fullList.size() <= m_maxNoTransOptPerCoverage)
					continue;
				
				// sort in vector
				nth_element(fullList.begin(), fullList.begin() + m_maxNoTransOptPerCoverage, fullList.end(), CompareTranslationOption);

				totalPruned += fullList.size() - m_maxNoTransOptPerCoverage;
				
				// delete the rest
				for (size_t i = m_maxNoTransOptPerCoverage ; i < fullList.size() ; ++i)
				{
					delete fullList[i];
				}
				fullList.resize(m_maxNoTransOptPerCoverage);
			}
		}
		VERBOSE(2,"       Total translation options: " << total << std::endl
			<< "Total translation options pruned: " << totalPruned << std::endl);
	}
}

/** Force a creation of a translation option where there are none for a particular source position.
* ie. where a source word has not been translated, create a translation option by
*				1. not observing the table limits on phrase/generation tables
*				2. using the handler ProcessUnknownWord()
* Call this function once translation option collection has been filled with translation options
*
* This function calls for unknown words is complicated by the fact it must handle different input types. 
* The call stack is
*		Base::ProcessUnknownWord()
*			Inherited::ProcessUnknownWord(position)
*				Base::ProcessOneUnknownWord()
*
* \param decodeStepList list of decoding steps
* \param factorCollection input sentence with all factors
*/

void TranslationOptionCollection::ProcessUnknownWord()
{
	size_t size = m_source.GetSize();

	// use just decode step (ie only translation) in m_collection
	for (size_t decodeStepId = 0 ; decodeStepId < m_collection.size() ; ++decodeStepId)
	{
		// try to translate for coverage with no trans by ignoring table limits
		for (size_t pos = 0 ; pos < size ; ++pos)
		{
				TranslationOptionList &fullList = GetTranslationOptionList(decodeStepId, pos, pos);
				size_t numTransOpt = fullList.size();
				if (numTransOpt == 0)
				{
					const DecodeStep &decodeStep = StaticData::Instance().GetDecodeStep(decodeStepId);
					CreateTranslationOptionsForRange(decodeStep, pos, pos, false);
				}
		}
			
		// create unknown words for 1 word coverage where we don't have any trans options
		for (size_t pos = 0 ; pos < size ; ++pos)
		{
			TranslationOptionList &fullList = GetTranslationOptionList(decodeStepId, pos, pos);
			if (fullList.size() == 0)
				ProcessUnknownWord(decodeStepId, pos);
		}
	}
}

/** special handling of ONE unknown words. Either add temporarily add word to translation table,
	* or drop the translation.
	* This function should be called by the ProcessOneUnknownWord() in the inherited class
	* At the moment, this unknown word handler is a bit of a hack, if copies over each factor from source
	* to target word, or uses the 'UNK' factor.
	* Ideally, this function should be in a class which can be expanded upon, for example, 
	* to create a morphologically aware handler. 
	*
	* \param sourceWord the unknown word
	* \param sourcePos
	* \param factorCollection input sentence with all factors
 */
void TranslationOptionCollection::ProcessOneUnknownWord(size_t decodeStepId, const Word &sourceWord
																												, size_t sourcePos)
{
	// unknown word, add as trans opt

		size_t isDigit = 0;
		if (StaticData::Instance().GetDropUnknown())
		{
			const Factor *f = sourceWord[0]; // TODO hack. shouldn't know which factor is surface
			const string &s = f->GetString();
			isDigit = s.find_first_of("0123456789");
			if (isDigit == string::npos) 
				isDigit = 0;
			else 
				isDigit = 1;
			// modify the starting bitmap
		}
		
		TranslationOption *transOpt;
		if (! StaticData::Instance().GetDropUnknown() || isDigit)
		{
			// add to dictionary
			TargetPhrase targetPhrase(Output);
			Word &targetWord = targetPhrase.AddWord();
						
			for (FactorType factorType = 0 ; factorType < MAX_NUM_FACTORS ; ++factorType)
			{
				const Factor *sourceFactor = sourceWord[factorType];
				if (sourceFactor == NULL)
					targetWord[factorType] = FactorCollection::Instance().AddFactor(Output, factorType, UNKNOWN_FACTOR);
				else
					targetWord[factorType] = FactorCollection::Instance().AddFactor(Output, factorType, sourceFactor->GetString());
			}
	
			targetPhrase.SetScore();
			targetPhrase.SetAlignment();
			
			transOpt = new TranslationOption(WordsRange(decodeStepId, sourcePos, sourcePos), targetPhrase, 0);
		}
		else 
		{ // drop source word. create blank trans opt
			const TargetPhrase targetPhrase(Output);
			transOpt = new TranslationOption(WordsRange(decodeStepId, sourcePos, sourcePos), targetPhrase, 0);
		}

		transOpt->CalcScore();
		Add(decodeStepId, transOpt);
}

/** compute future score matrix in a dynamic programming fashion.
	* This matrix used in search.
	* Call this function once translation option collection has been filled with translation options
*/
void TranslationOptionCollection::CalcFutureScore()
{
  // setup the matrix (ignore lower triangle, set upper triangle to -inf
  size_t size = m_source.GetSize(); // the width of the matrix

  // walk all the translation options and record the cheapest option for each span
	for (size_t decodeStepId = 0 ; decodeStepId < m_collection.size() ; ++decodeStepId)
	{	
		for (size_t startPos = 0 ; startPos < m_source.GetSize() ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < m_source.GetSize() ; ++endPos)
			{
				TranslationOptionList &transOptList = GetTranslationOptionList(decodeStepId, startPos, endPos);

				TranslationOptionList::const_iterator iterTransOpt;
				for(iterTransOpt = transOptList.begin() ; iterTransOpt != transOptList.end() ; ++iterTransOpt) 
				{
					const TranslationOption &transOpt = **iterTransOpt;
					float score = transOpt.GetFutureScore();
					if (score > m_futureScore.GetScore(decodeStepId, startPos, endPos))
						m_futureScore.SetScore(decodeStepId, startPos, endPos, score);
				}
			}
		}
	}

	// calc best score for a given span
	m_futureScore.CalcOptimisticScores();

	IFVERBOSE(3)
	{		
		for (size_t decodeStepId = 0 ; decodeStepId < m_collection.size() ; ++decodeStepId)
		{
			size_t total = 0;
			for(size_t row=0; row<size; row++)
			{
				for(size_t col=row; col<size; col++)
				{
      		size_t count = GetTranslationOptionList(decodeStepId, row, col).size();
					TRACE_ERR( "translation options spanning from  "
        					<< row <<" to "<< col <<" is "
        					<< count <<endl);
     			total += count;
				}
			}
			TRACE_ERR( "translation options generated in total: "<< total << endl);

			for(size_t row=0; row<size; row++)
			{
				for(size_t col=row; col<size; col++)
					TRACE_ERR( "future cost from "<< row <<" to "<< col <<" is "<< m_futureScore.GetScore(decodeStepId, row, col) <<endl);
			}
		}
	}
}



/** Create all possible translations from the phrase tables
 * for a particular input sentence. This implies applying all
 * translation and generation steps. Also computes future cost matrix.
 * \param decodeStepList list of decoding steps
 * \param factorCollection input sentence with all factors
 */
void TranslationOptionCollection::CreateTranslationOptions(const vector<DecodeStep*> &decodeStepList)
{	
	// resize trans opt collection for each decode step
	m_collection.resize(decodeStepList.size());

	// create map of future score matrices
	m_futureScore.Initialize(decodeStepList);

	vector<DecodeStep*>::const_iterator iterDecodeStep;
	for (iterDecodeStep = decodeStepList.begin() ; iterDecodeStep != decodeStepList.end() ; ++iterDecodeStep)
	{
		DecodeStep &decodeStep = **iterDecodeStep;

 		if (decodeStep.GetDecodeType() == Translate)
		{
			// create map of matrices for each trans step
			TransOptMatrix &transOptMatrix = m_collection[decodeStep.GetId()];
			
			// create 2-d vector
			size_t size = m_source.GetSize();
			for (size_t startPos = 0 ; startPos < size ; ++startPos)
			{
				transOptMatrix.push_back( vector< TranslationOptionList >() );
				for (size_t endPos = startPos ; endPos < size ; ++endPos)
				{
					transOptMatrix[startPos].push_back( TranslationOptionList() );
				}
			}
		
			// loop over all substrings of the source sentence, look them up
			// in the phraseDictionary (which is the- possibly filtered-- phrase
			// table loaded on initialization), generate TranslationOption objects
			// for all phrases
			
			for (size_t startPos = 0 ; startPos < m_source.GetSize() ; startPos++)
			{
				for (size_t endPos = startPos ; endPos < m_source.GetSize() ; endPos++)
				{
					CreateTranslationOptionsForRange( decodeStep, startPos, endPos, true);
				}
			}
		}
	}

	ProcessUnknownWord();
	
	// Prune
	Prune();

	// future score matrix
	CalcFutureScore();
}

/** create translation options that exactly cover a specific input span. 
 * Called by CreateTranslationOptions() and ProcessUnknownWord()
 * \param decodeStepList list of decoding steps
 * \param factorCollection input sentence with all factors
 * \param startPos first position in input sentence
 * \param lastPos last position in input sentence 
 * \param adhereTableLimit whether phrase & generation table limits are adhered to
 */
void TranslationOptionCollection::CreateTranslationOptionsForRange(
																													 const DecodeStep &decodeStep
																													 , size_t startPos
																													 , size_t endPos
																													 , bool adhereTableLimit)
{
	// partial trans opt stored in here
	PartialTranslOptColl transOptColl;
	
	// initial translation step
	static_cast<const DecodeStepTranslation&>(decodeStep).Process(
													m_source, transOptColl, startPos, endPos, adhereTableLimit );

	// add to fully formed translation option list
	const vector<TranslationOption*>& partTransOptList = transOptColl.GetList();
	vector<TranslationOption*>::const_iterator iterColl;
	for (iterColl = partTransOptList.begin() ; iterColl != partTransOptList.end() ; ++iterColl)
	{
		TranslationOption *transOpt = *iterColl;
		transOpt->CalcScore();
		Add(decodeStep.GetId(), transOpt);
	}
}

/** add translation option to the list
 * \param translationOption translation option to be added */
void TranslationOptionCollection::Add(size_t decodeStepId, const TranslationOption *translationOption)
{
	const WordsRange &coverage = translationOption->GetSourceWordsRange();
	TransOptMatrix &transOptMatrix = m_collection[decodeStepId];
	transOptMatrix[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()].push_back(translationOption);
}

TO_STRING_BODY(TranslationOptionCollection);

