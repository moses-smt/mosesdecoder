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
	,m_maxNoTransOptPerCoverage(maxNoTransOptPerCoverage)
{
}

/** destructor, clears out data structures */
TranslationOptionCollection::~TranslationOptionCollection()
{
	std::map<const DecodeStep*, TransOptMatrix>::iterator iterMap;

	for (iterMap = m_collection.begin() ; iterMap != m_collection.end() ; ++iterMap)
	{
		const DecodeStep *decodeStep = iterMap->first;
		// delete all trans opt
		size_t size = m_source.GetSize();
		for (size_t startPos = 0 ; startPos < size ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < size ; ++endPos)
			{
				RemoveAllInColl(GetTranslationOptionList(decodeStep, startPos, endPos));
			}
		}

		// future cost matrix
		delete m_futureScore[decodeStep];
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

	std::map<const DecodeStep*, TransOptMatrix>::iterator iterMap;
	for (iterMap = m_collection.begin() ; iterMap != m_collection.end() ; ++iterMap)
	{
		const DecodeStep *decodeStep = iterMap->first;

		size_t total = 0;
		size_t totalPruned = 0;
		for (size_t startPos = 0 ; startPos < size ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < size ; ++endPos)
			{
				TranslationOptionList &fullList = GetTranslationOptionList(decodeStep, startPos, endPos);
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

void TranslationOptionCollection::ProcessUnknownWord(FactorCollection &factorCollection)
{
	size_t size = m_source.GetSize();

	// use just decode step (ie only translation) in m_collection
	std::map<const DecodeStep*, TransOptMatrix>::iterator iterMap;
	for (iterMap = m_collection.begin() ; iterMap != m_collection.end() ; ++iterMap)
	{
		const DecodeStep *decodeStep = iterMap->first;

		// try to translate for coverage with no trans by ignoring table limits
		for (size_t pos = 0 ; pos < size ; ++pos)
		{
				TranslationOptionList &fullList = GetTranslationOptionList(decodeStep, pos, pos);
				size_t numTransOpt = fullList.size();
				if (numTransOpt == 0)
				{
					CreateTranslationOptionsForRange(decodeStep, factorCollection
																				, pos, pos, false);
				}
		}
			
		// create unknown words for 1 word coverage where we don't have any trans options
		for (size_t pos = 0 ; pos < size ; ++pos)
		{
			TranslationOptionList &fullList = GetTranslationOptionList(decodeStep, pos, pos);
			if (fullList.size() == 0)
				ProcessUnknownWord(decodeStep, pos, *m_factorCollection);
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
void TranslationOptionCollection::ProcessOneUnknownWord(const DecodeStep *decodeStep, const Word &sourceWord
																												, size_t sourcePos, FactorCollection &factorCollection)
{
	// unknown word, add as trans opt

		size_t isDigit = 0;
		if (StaticData::Instance()->GetDropUnknown())
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
		if (! StaticData::Instance()->GetDropUnknown() || isDigit)
		{
			// add to dictionary
			TargetPhrase targetPhrase(Output);
			Word &targetWord = targetPhrase.AddWord();
						
			for (unsigned int currFactor = 0 ; currFactor < MAX_NUM_FACTORS ; currFactor++)
			{
				FactorType factorType = static_cast<FactorType>(currFactor);
				
				const Factor *sourceFactor = sourceWord[currFactor];
				if (sourceFactor == NULL)
					targetWord[factorType] = factorCollection.AddFactor(Output, factorType, UNKNOWN_FACTOR);
				else
					targetWord[factorType] = factorCollection.AddFactor(Output, factorType, sourceFactor->GetString());
			}
	
			targetPhrase.SetScore();
			
			transOpt = new TranslationOption(WordsRange(decodeStep, sourcePos, sourcePos), targetPhrase, 0);
		}
		else 
		{ // drop source word. create blank trans opt
			const TargetPhrase targetPhrase(Output);
			transOpt = new TranslationOption(WordsRange(decodeStep, sourcePos, sourcePos), targetPhrase, 0);
		}

		transOpt->CalcScore();
		Add(decodeStep, transOpt);
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
	std::map<const DecodeStep*, TransOptMatrix>::iterator iterMap;

	for (iterMap = m_collection.begin() ; iterMap != m_collection.end() ; ++iterMap)
	{
		const DecodeStep *decodeStep = iterMap->first;
		SquareMatrix &squareMatrix = GetFutureScore(decodeStep);
		squareMatrix.ResetScore(-numeric_limits<float>::infinity());
		//m_futureScore.ResetScore(0);

		for (size_t startPos = 0 ; startPos < m_source.GetSize() ; ++startPos)
		{
			for (size_t endPos = startPos ; endPos < m_source.GetSize() ; ++endPos)
			{
				TranslationOptionList &transOptList = GetTranslationOptionList(decodeStep, startPos, endPos);

				TranslationOptionList::const_iterator iterTransOpt;
				for(iterTransOpt = transOptList.begin() ; iterTransOpt != transOptList.end() ; ++iterTransOpt) 
				{
					const TranslationOption &transOpt = **iterTransOpt;
					float score = transOpt.GetFutureScore();
					if (score > squareMatrix.GetScore(startPos, endPos))
						squareMatrix.SetScore(startPos, endPos, score);
				}
			}
		}
	}
  // now fill all the cells in the strictly upper triangle
  //   there is no way to modify the diagonal now, in the case
  //   where no translation option covers a single-word span,
  //   we leave the +inf in the matrix
  // like in chart parsing we want each cell to contain the highest score
  // of the full-span trOpt or the sum of scores of joining two smaller spans

	for (iterMap = m_collection.begin() ; iterMap != m_collection.end() ; ++iterMap)
	{
		const DecodeStep *decodeStep = iterMap->first;
		SquareMatrix &squareMatrix = GetFutureScore(decodeStep);

		for(size_t colstart = 1; colstart < size ; colstart++) 
		{
			for(size_t diagshift = 0; diagshift < size-colstart ; diagshift++) 
			{
				size_t startPos = diagshift;
				size_t endPos = colstart+diagshift;
				for(size_t joinAt = startPos; joinAt < endPos ; joinAt++)  {
					float joinedScore = squareMatrix.GetScore(startPos, joinAt)
														+ squareMatrix.GetScore(joinAt+1, endPos);
					/* // uncomment to see the cell filling scheme
					TRACE_ERR( "[" <<startPos<<","<<endPos<<"] <-? ["<<startPos<<","<<joinAt<<"]+["<<joinAt+1<<","<<endPos
						<< "] (colstart: "<<colstart<<", diagshift: "<<diagshift<<")"<<endl);
					*/
					if (joinedScore > squareMatrix.GetScore(startPos, endPos))
						squareMatrix.SetScore(startPos, endPos, joinedScore);
				}
			}
		}
	}

	IFVERBOSE(3)
	{		
		for (iterMap = m_collection.begin() ; iterMap != m_collection.end() ; ++iterMap)
		{
			const DecodeStep *decodeStep = iterMap->first;
			SquareMatrix &squareMatrix = GetFutureScore(decodeStep);

			size_t total = 0;
			for(size_t row=0; row<size; row++)
			{
				for(size_t col=row; col<size; col++)
				{
      		size_t count = GetTranslationOptionList(decodeStep, row, col).size();
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
					TRACE_ERR( "future cost from "<< row <<" to "<< col <<" is "<< squareMatrix.GetScore(row, col) <<endl);
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
void TranslationOptionCollection::CreateTranslationOptions(const list < DecodeStep* > &decodeStepList
																													 , FactorCollection &factorCollection)
{
	m_factorCollection = &factorCollection;
	
	list < DecodeStep* >::const_iterator iterDecodeStep;
	for (iterDecodeStep = decodeStepList.begin() ; iterDecodeStep != decodeStepList.end() ; ++iterDecodeStep)
	{
		DecodeStep *decodeStep = *iterDecodeStep;

		if (decodeStep->GetDecodeType() == Translate)
		{
			// create map of matrices for each trans step
			TransOptMatrix &transOptMatrix = m_collection[decodeStep];
			
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
		
			// create map of future score matrices
			m_futureScore[decodeStep] = new SquareMatrix(m_source.GetSize());

			// loop over all substrings of the source sentence, look them up
			// in the phraseDictionary (which is the- possibly filtered-- phrase
			// table loaded on initialization), generate TranslationOption objects
			// for all phrases
			
			for (size_t startPos = 0 ; startPos < m_source.GetSize() ; startPos++)
			{
				for (size_t endPos = startPos ; endPos < m_source.GetSize() ; endPos++)
				{
					CreateTranslationOptionsForRange( decodeStep, factorCollection, startPos, endPos, true);
				}
			}
		}
	}

	ProcessUnknownWord(factorCollection);
	
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
																													 const DecodeStep *decodeStep
																													 , FactorCollection &factorCollection
																													 , size_t startPos
																													 , size_t endPos
																													 , bool adhereTableLimit)
{
	// partial trans opt stored in here
	PartialTranslOptColl transOptColl;
	
	// initial translation step
	static_cast<const DecodeStepTranslation&>(*decodeStep).ProcessInitialTranslation(m_source, factorCollection
														, transOptColl, startPos, endPos, adhereTableLimit );

	// add to fully formed translation option list
	const vector<TranslationOption*>& partTransOptList = transOptColl.GetList();
	vector<TranslationOption*>::const_iterator iterColl;
	for (iterColl = partTransOptList.begin() ; iterColl != partTransOptList.end() ; ++iterColl)
	{
		TranslationOption *transOpt = *iterColl;
		transOpt->CalcScore();
		Add(decodeStep, transOpt);
	}
}

/** add translation option to the list
 * \param translationOption translation option to be added */
void TranslationOptionCollection::Add(const DecodeStep *decodeStep, const TranslationOption *translationOption)
{
	const WordsRange &coverage = translationOption->GetSourceWordsRange();
	TransOptMatrix &transOptMatrix = m_collection[decodeStep];
	transOptMatrix[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()].push_back(translationOption);
}

TO_STRING_BODY(TranslationOptionCollection);

