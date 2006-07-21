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
// $Id$

#include "TranslationOptionCollection.h"
#include "Input.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"

using namespace std;

TranslationOptionCollection::TranslationOptionCollection(InputType const& src)
	: m_source(src),m_futureScore(src.GetSize()),m_initialCoverage(src.GetSize())
{
}

TranslationOptionCollection::~TranslationOptionCollection() {}

size_t TranslationOptionCollection::GetSize() const {return m_source.GetSize();}

void TranslationOptionCollection::
CreateTranslationOptions(const std::list < DecodeStep > &decodeStepList
												 , const LMList & //languageModels
												 , const LMList & allLM
												 , FactorCollection & factorCollection
												 , float weightWordPenalty
												 , bool dropUnknown
												 , size_t verboseLevel)
{
  // loop over all substrings of the source sentence, look them up
  // in the phraseDictionary (which is the- possibly filtered-- phrase
  // table loaded on initialization), generate TranslationOption objects
  // for all phrases
  //
  // possible optimization- don't consider phrases longer than the longest
  // phrase in the PhraseDictionary?
  
	PhraseDictionaryBase &dictionary= decodeStepList.front().GetPhraseDictionary();

  typedef std::vector<TargetPhraseCollection const*> vTPC;
  std::vector<vTPC> mTPC(m_source.GetSize(),vTPC(m_source.GetSize(),0));
  size_t maxLen=0;

  for (size_t startPos = 0 ; startPos < m_source.GetSize() ; ++startPos)
		for (size_t endPos = startPos ; endPos < m_source.GetSize() ; ++endPos)
			{
				WordsRange wordsRange(startPos, endPos);

				const TargetPhraseCollection *phraseColl= dictionary.GetTargetPhraseCollection(m_source,wordsRange);
				//				const TargetPhraseCollection *phraseColl= m_source.CreateTargetPhraseCollection(dictionary,wordsRange);

				mTPC[startPos][endPos]=phraseColl;
				if (phraseColl != NULL)
					{
						maxLen=std::max(endPos-startPos+1,maxLen);
						if (verboseLevel >= 3) 
							std::cout << "[" << m_source.GetSubString(wordsRange) << "; " 
												<< startPos << "-" << endPos << "]\n";
		
						for(TargetPhraseCollection::const_iterator iterTargetPhrase=phraseColl->begin(); 
								iterTargetPhrase != phraseColl->end() ; ++iterTargetPhrase)
							{
								TranslationOption transOpt(wordsRange, *iterTargetPhrase);
								push_back(transOpt);
								if (verboseLevel >= 3) std::cout << "\t" << transOpt << "\n";
							}
						if (verboseLevel >= 3) std::cout << std::endl;
					}
				else if (wordsRange.GetWordsCount() == 1)
					{
						// handle unknown word

						if(!HandleUnkownWord(dictionary,
																 startPos,
																 factorCollection,
																 allLM,
																 dropUnknown,
																 weightWordPenalty))
							// drop unk
							m_initialCoverage.SetValue(startPos, startPos,1); 
					}
			}

	ComputeFutureScores(verboseLevel);
}
	  
void TranslationOptionCollection::ComputeFutureScores(size_t verboseLevel) 
{

#if 1

  // create future score matrix
  // for each span in the source phrase (denoted by start and end)
  for(size_t startPos = 0; startPos < m_source.GetSize() ; startPos++) 
    {
      for(size_t endPos = startPos; endPos < m_source.GetSize() ; endPos++) 
				{
					size_t length = endPos - startPos + 1;
					std::vector< float > score(length + 1);
					score[0] = 0;
					for(size_t currLength = 1 ; currLength <= length ; currLength++) 
						// initalize their future cost to -infinity
						{
							score[currLength] = - std::numeric_limits<float>::infinity();
						}

					for(size_t currLength = 0 ; currLength < length ; currLength++) 
						{
							// iterate over possible translations of this source subphrase and
							// keep track of the highest cost option
							TranslationOptionCollection::const_iterator iterTransOpt;
							for(iterTransOpt = begin() ; iterTransOpt != end() ; ++iterTransOpt)
								{
									const TranslationOption &transOpt = *iterTransOpt;
									size_t index = currLength + transOpt.GetSize();

									if (transOpt.GetStartPos() == currLength + startPos 
											&& transOpt.GetEndPos() <= endPos 
											&& transOpt.GetFutureScore() + score[currLength] > score[index]) 
										{
											score[index] = transOpt.GetFutureScore() + score[currLength];
										}
								}
						}
					// record the highest cost option in the future cost table.
					m_futureScore.SetScore(startPos, endPos, score[length]);

					//print information about future cost table when verbose option is set

					if(verboseLevel > 0) 
						{		
							std::cout<<"future cost from "<<startPos<<" to "<<endPos<<" is "<<score[length]<<std::endl;
						}
				}
    }


#else

	// alternative implementation, DOES NOT WORK (yet)!

	// create future score matrix
	// for each span in the source phrase (denoted by start and end)

	  
	// init future costs
	for(size_t endPos=0; endPos<m_source.GetSize(); ++endPos) 
		for(size_t startPos=0; startPos<=endPos; ++startPos)   
			{
				float currScore=-std::numeric_limits<float>().infinity();
				if(TargetPhraseCollection const *p=mTPC[startPos][endPos]) 
					for(TargetPhraseCollection::const_iterator i=p->begin();i!=p->end();++i)
						currScore=std::max(currScore,i->GetFutureScore());
				m_futureScore.SetScore(startPos,endPos,currScore);
			}
		  
	// solve DP recursion, similar to CYK parsing
	for(size_t len=1;len<maxLen;++len)
		for(size_t startPos=0; startPos<m_source.GetSize()-len; ++startPos)
			{
			  size_t endPos=startPos+len;
			  float currScore=m_futureScore.GetScore(startPos,endPos);
			  for(size_t k=startPos;k<endPos;++k)
					currScore=std::max(currScore,
														 m_futureScore.GetScore(startPos,k) 
														 + m_futureScore.GetScore(k+1,endPos));
				m_futureScore.SetScore(startPos,endPos,currScore);

				if(verboseLevel > 0) 
					std::cout<<"future cost from "<<startPos<<" to "<<endPos<<" is "
									 <<m_futureScore.GetScore(startPos,endPos)<<std::endl;
			}
#endif		  
			  
}

