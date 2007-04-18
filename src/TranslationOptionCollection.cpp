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
#include "DecodeStep.h"
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
	,m_futureScore(src.GetSize())
	,m_maxNoTransOptPerCoverage(maxNoTransOptPerCoverage)
{
	// create 2-d vector
	size_t size = src.GetSize();
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		m_collection.push_back( vector< TranslationOptionList >() );
		for (size_t endPos = startPos ; endPos < size ; ++endPos)
		{
			m_collection[startPos].push_back( TranslationOptionList() );
		}
	}
}

/** destructor, clears out data structures */
TranslationOptionCollection::~TranslationOptionCollection()
{
	// delete all trans opt
	size_t size = m_source.GetSize();
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < size ; ++endPos)
		{
		 RemoveAllInColl(GetTranslationOptionList(startPos, endPos));
		}
	}
	RemoveAllInColl(m_unksrcs);
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

	size_t total = 0;
	size_t totalPruned = 0;
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < size ; ++endPos)
		{
			TranslationOptionList &fullList = GetTranslationOptionList(startPos, endPos);
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

void TranslationOptionCollection::ProcessUnknownWord(const std::vector < std::list <const DecodeStep* > * > &decodeStepVL)
{
	size_t size = m_source.GetSize();
	// try to translation for coverage with no trans by expanding table limit
	for (size_t startVL = 0 ; startVL < decodeStepVL.size() ; startVL++) 
	{
	  const list <const DecodeStep* > * decodeStepList = decodeStepVL[startVL];
		for (size_t pos = 0 ; pos < size ; ++pos)
		{
				TranslationOptionList &fullList = GetTranslationOptionList(pos, pos);
				size_t numTransOpt = fullList.size();
				if (numTransOpt == 0)
				{
					CreateTranslationOptionsForRange(*decodeStepList
																				, pos, pos, false);
				}
		}
	}
		
	// create unknown words for 1 word coverage where we don't have any trans options
	for (size_t pos = 0 ; pos < size ; ++pos)
	{
		TranslationOptionList &fullList = GetTranslationOptionList(pos, pos);
		if (fullList.size() == 0)
			ProcessUnknownWord(pos);
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
void TranslationOptionCollection::ProcessOneUnknownWord(const Word &sourceWord,
																														size_t sourcePos, size_t length)
{
	// unknown word, add as trans opt
	FactorCollection &factorCollection = FactorCollection::Instance();

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
	Phrase* m_unksrc = new Phrase(Input); m_unksrc->AddWord() = sourceWord;
	m_unksrcs.push_back(m_unksrc);
	
	TranslationOption *transOpt;
	if (! StaticData::Instance().GetDropUnknown() || isDigit)
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
		targetPhrase.SetSourcePhrase(m_unksrc);
		transOpt = new TranslationOption(WordsRange(sourcePos, sourcePos + length - 1), targetPhrase, m_source, 0);	
	}
	else 
	{ // drop source word. create blank trans opt
		TargetPhrase targetPhrase(Output);
		targetPhrase.SetSourcePhrase(m_unksrc);
		transOpt = new TranslationOption(WordsRange(sourcePos, sourcePos + length - 1), targetPhrase, m_source, 0);
	}

	transOpt->CalcScore();
	Add(transOpt);
}

/** compute future score matrix in a dynamic programming fashion.
	* This matrix used in search.
	* Call this function once translation option collection has been filled with translation options
*/
void TranslationOptionCollection::CalcFutureScore()
{
  // setup the matrix (ignore lower triangle, set upper triangle to -inf
  size_t size = m_source.GetSize(); // the width of the matrix

  for(size_t row=0; row<size; row++) {
    for(size_t col=row; col<size; col++) {
      m_futureScore.SetScore(row, col, -numeric_limits<float>::infinity());
    }
  }

  // walk all the translation options and record the cheapest option for each span
	for (size_t startPos = 0 ; startPos < m_source.GetSize() ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < m_source.GetSize() ; ++endPos)
		{
			TranslationOptionList &transOptList = GetTranslationOptionList(startPos, endPos);

			TranslationOptionList::const_iterator iterTransOpt;
			for(iterTransOpt = transOptList.begin() ; iterTransOpt != transOptList.end() ; ++iterTransOpt) 
			{
				const TranslationOption &transOpt = **iterTransOpt;
				float score = transOpt.GetFutureScore();
				if (score > m_futureScore.GetScore(startPos, endPos))
					m_futureScore.SetScore(startPos, endPos, score);
			}
		}
	}

  // now fill all the cells in the strictly upper triangle
  //   there is no way to modify the diagonal now, in the case
  //   where no translation option covers a single-word span,
  //   we leave the +inf in the matrix
  // like in chart parsing we want each cell to contain the highest score
  // of the full-span trOpt or the sum of scores of joining two smaller spans

	for(size_t colstart = 1; colstart < size ; colstart++) {
		for(size_t diagshift = 0; diagshift < size-colstart ; diagshift++) {
            size_t startPos = diagshift;
            size_t endPos = colstart+diagshift;
			for(size_t joinAt = startPos; joinAt < endPos ; joinAt++)  {
              float joinedScore = m_futureScore.GetScore(startPos, joinAt)
                                + m_futureScore.GetScore(joinAt+1, endPos);
              /* // uncomment to see the cell filling scheme
              TRACE_ERR( "[" <<startPos<<","<<endPos<<"] <-? ["<<startPos<<","<<joinAt<<"]+["<<joinAt+1<<","<<endPos
                << "] (colstart: "<<colstart<<", diagshift: "<<diagshift<<")"<<endl);
              */
              if (joinedScore > m_futureScore.GetScore(startPos, endPos))
                m_futureScore.SetScore(startPos, endPos, joinedScore);
            }
        }
    }

	IFVERBOSE(3)
	{		
      int total = 0;
      for(size_t row=0; row<size; row++)
      {
        for(size_t col=row; col<size; col++)
        {
        	int count = GetTranslationOptionList(row, col).size();
	        TRACE_ERR( "translation options spanning from  "
	        				<< row <<" to "<< col <<" is "
	        				<< count <<endl);
       		total += count;
        }
      }
      TRACE_ERR( "translation options generated in total: "<< total << endl);

      for(size_t row=0; row<size; row++)
        for(size_t col=row; col<size; col++)
					TRACE_ERR( "future cost from "<< row <<" to "<< col <<" is "<< m_futureScore.GetScore(row, col) <<endl);
    }
}



/** Create all possible translations from the phrase tables
 * for a particular input sentence. This implies applying all
 * translation and generation steps. Also computes future cost matrix.
 * \param decodeStepList list of decoding steps
 * \param factorCollection input sentence with all factors
 */
void TranslationOptionCollection::CreateTranslationOptions(const vector <list <const DecodeStep* > * > &decodeStepVL)
{	
	// loop over all substrings of the source sentence, look them up
	// in the phraseDictionary (which is the- possibly filtered-- phrase
	// table loaded on initialization), generate TranslationOption objects
	// for all phrases
	for (size_t startVL = 0 ; startVL < decodeStepVL.size() ; startVL++) 
	{
	  const list <const DecodeStep* > * decodeStepList = decodeStepVL[startVL];
		for (size_t startPos = 0 ; startPos < m_source.GetSize() ; startPos++)
		{
			for (size_t endPos = startPos ; endPos < m_source.GetSize() ; endPos++)
			{
				CreateTranslationOptionsForRange( *decodeStepList, startPos, endPos, true);				
 			}
		}
	}

	VERBOSE(3,"Translation Option Collection\n " << *this << endl);
	
	ProcessUnknownWord(decodeStepVL);
	
	// Prune
	Prune();

	// future score matrix
	CalcFutureScore();

	// Cached lex reodering costs
	CacheLexReordering();
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
																													 const list <const DecodeStep* > &decodeStepList
																													 , size_t startPos
																													 , size_t endPos
																													 , bool adhereTableLimit)
{
	// partial trans opt stored in here
	PartialTranslOptColl* oldPtoc = new PartialTranslOptColl;
	
	// initial translation step
	list <const DecodeStep* >::const_iterator iterStep = decodeStepList.begin();
	const DecodeStep &decodeStep = **iterStep;

	ProcessInitialTranslation(decodeStep, *oldPtoc
														, startPos, endPos, adhereTableLimit );

	// do rest of decode steps
	size_t totalEarlyPruned = 0;
	int indexStep = 0;
	for (++iterStep ; iterStep != decodeStepList.end() ; ++iterStep) 
		{
			const DecodeStep &decodeStep = **iterStep;
			PartialTranslOptColl* newPtoc = new PartialTranslOptColl;

			// go thru each intermediate trans opt just created
			const vector<TranslationOption*>& partTransOptList = oldPtoc->GetList();
			vector<TranslationOption*>::const_iterator iterPartialTranslOpt;
			for (iterPartialTranslOpt = partTransOptList.begin() ; iterPartialTranslOpt != partTransOptList.end() ; ++iterPartialTranslOpt)
			{
				TranslationOption &inputPartialTranslOpt = **iterPartialTranslOpt;
				decodeStep.Process(inputPartialTranslOpt
																	 , decodeStep
																	 , *newPtoc
																	 , this
																	 , adhereTableLimit);
			}
			// last but 1 partial trans not required anymore
			totalEarlyPruned += newPtoc->GetPrunedCount();
			delete oldPtoc;
			oldPtoc = newPtoc;
			indexStep++;
		} // for (++iterStep 

	// add to fully formed translation option list
	PartialTranslOptColl &lastPartialTranslOptColl	= *oldPtoc;
	const vector<TranslationOption*>& partTransOptList = lastPartialTranslOptColl.GetList();
	vector<TranslationOption*>::const_iterator iterColl;
	for (iterColl = partTransOptList.begin() ; iterColl != partTransOptList.end() ; ++iterColl)
		{
			TranslationOption *transOpt = *iterColl;
			transOpt->CalcScore();
			Add(transOpt);
		}

	lastPartialTranslOptColl.DetachAll();
	totalEarlyPruned += oldPtoc->GetPrunedCount();
	delete oldPtoc;
	// TRACE_ERR( "Early translation options pruned: " << totalEarlyPruned << endl);
}

/** initialize list of partial translation options by applying the first translation step 
	* Ideally, this function should be in DecodeStepTranslation class
	*/
void TranslationOptionCollection::ProcessInitialTranslation(
															const DecodeStep &decodeStep
															, PartialTranslOptColl &outputPartialTranslOptColl
															, size_t startPos
															, size_t endPos
															, bool adhereTableLimit)
{
	const PhraseDictionary &phraseDictionary = decodeStep.GetPhraseDictionary();
	const size_t tableLimit = phraseDictionary.GetTableLimit();

	const WordsRange wordsRange(startPos, endPos);
	const TargetPhraseCollection *phraseColl =	phraseDictionary.GetTargetPhraseCollection(m_source,wordsRange); 

	if (phraseColl != NULL)
	{
		VERBOSE(3,"[" << m_source.GetSubString(wordsRange) << "; " << startPos << "-" << endPos << "]\n");
			
		TargetPhraseCollection::const_iterator iterTargetPhrase, iterEnd;
		iterEnd = (!adhereTableLimit || tableLimit == 0 || phraseColl->GetSize() < tableLimit) ? phraseColl->end() : phraseColl->begin() + tableLimit;
		
		for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != iterEnd ; ++iterTargetPhrase)
		{
			const TargetPhrase	&targetPhrase = **iterTargetPhrase;
			outputPartialTranslOptColl.Add ( new TranslationOption(wordsRange, targetPhrase, m_source) );
			
			VERBOSE(3,"\t" << targetPhrase << "\n");
		}
		VERBOSE(3,endl);
	}
}

/** add translation option to the list
 * \param translationOption translation option to be added */
void TranslationOptionCollection::Add(TranslationOption *translationOption)
{
	const WordsRange &coverage = translationOption->GetSourceWordsRange();
	m_collection[coverage.GetStartPos()][coverage.GetEndPos() - coverage.GetStartPos()].push_back(translationOption);
}

TO_STRING_BODY(TranslationOptionCollection);

inline std::ostream& operator<<(std::ostream& out, const TranslationOptionCollection& coll)
{
	size_t size = coll.GetSize();
	for (size_t startPos = 0 ; startPos < size ; ++startPos)
	{
		for (size_t endPos = startPos ; endPos < size ; ++endPos)
		{
			TranslationOptionList fullList = coll.GetTranslationOptionList(startPos, endPos);
			size_t sizeFull = fullList.size();
		  for (size_t i = 0; i < sizeFull; i++) 
			{
			  out << *fullList[i] << std::endl;
			}
		}
	}
 
  //std::vector< std::vector< TranslationOptionList > >::const_iterator i = coll.m_collection.begin();
	//size_t j = 0;
	//for (; i!=coll.m_collection.end(); ++i) {
    //out << "s[" << j++ << "].size=" << i->size() << std::endl;
	//}

	return out;
}

void TranslationOptionCollection::CacheLexReordering()
{
	const std::vector<LexicalReordering*> &lexReorderingModels = StaticData::Instance().GetReorderModels();

	std::vector<LexicalReordering*>::const_iterator iterLexreordering;

	for (iterLexreordering = lexReorderingModels.begin() ; iterLexreordering != lexReorderingModels.end() ; ++iterLexreordering)
	{
		LexicalReordering &lexreordering = **iterLexreordering;

		for (size_t startPos = 0 ; startPos < m_source.GetSize() ; startPos++)
		{
			for (size_t endPos = startPos ; endPos < m_source.GetSize() ; endPos++)
			{
				TranslationOptionList &transOptList = GetTranslationOptionList( startPos, endPos);				
				TranslationOptionList::iterator iterTransOpt;
				for(iterTransOpt = transOptList.begin() ; iterTransOpt != transOptList.end() ; ++iterTransOpt) 
				{
					TranslationOption &transOpt = **iterTransOpt;
					const Phrase *sourcePhrase = transOpt.GetSourcePhrase();
					if (sourcePhrase)
					{
						Score score = lexreordering.GetProb(*sourcePhrase
																							, transOpt.GetTargetPhrase());
						// TODO should have better handling of unknown reordering entries
						if (!score.empty())
							transOpt.CacheReorderingProb(lexreordering, score);
					}
				}
			}
		}
	}
}
