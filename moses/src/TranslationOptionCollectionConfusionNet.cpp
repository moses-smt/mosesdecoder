// $Id$
#include "TranslationOptionCollectionConfusionNet.h"
#include "ConfusionNet.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"

TranslationOptionCollectionConfusionNet::
TranslationOptionCollectionConfusionNet(const ConfusionNet &input) 
	: TranslationOptionCollection(input) {}
#if 0
size_t TranslationOptionCollectionConfusionNet::GetSourceSize() const 
{
	return m_source.GetSize();
}

void TranslationOptionCollectionConfusionNet::
CreateTranslationOptions(const std::list < DecodeStep > &decodeStepList
												 , const LMList & //languageModels
												 , const LMList & //allLM
												 , FactorCollection & //factorCollection
												 , float //weightWordPenalty
												 , bool //dropUnknown
												 , size_t verboseLevel)
{
  // loop over all substrings of the source sentence, look them up
  // in the phraseDictionary (which is the- possibly filtered-- phrase
  // table loaded on initialization), generate TranslationOption objects
  // for all phrases
  //
  // possible optimization- don't consider phrases longer than the longest
  // phrase in the PhraseDictionary?
  
  Dictionary *dictionary = decodeStepList.front().GetDictionaryPtr();
  
  typedef std::vector<TargetPhraseCollection const*> vTPC;
  std::vector<vTPC> mTPC(m_source.GetSize(),vTPC(m_source.GetSize(),0));
  size_t maxLen=0;

  for (size_t startPos = 0 ; startPos < m_source.GetSize() ; ++startPos)
		for (size_t endPos = startPos ; endPos < m_source.GetSize() ; ++endPos)
			{
				WordsRange wordsRange(startPos, endPos);
				const TargetPhraseCollection *phraseColl= 
					CreateTargetPhraseCollection(dictionary,&m_source,wordsRange);
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
						// drop unk
						m_initialCoverage.SetValue(startPos, startPos,1); 
					}
			}

	  
	  
	  
	  
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
			  
			  
}

#endif
