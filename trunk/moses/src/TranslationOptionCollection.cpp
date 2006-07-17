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

#include "TranslationOptionCollection.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "LanguageModel.h"
#include "PhraseDictionary.h"
#include "FactorCollection.h"

using namespace std;

TranslationOptionCollection::TranslationOptionCollection(const Sentence &inputSentence)
	: m_inputSentence(inputSentence)
	,m_futureScore(inputSentence.GetSize())
	,m_initialCoverage(inputSentence.GetSize())
  {
  }

void TranslationOptionCollection::CreateTranslationOptions(
  														const list < DecodeStep > &decodeStepList
  														, const LMList &languageModels
  														, const LMList &allLM
  														, FactorCollection &factorCollection
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
  
  PhraseDictionary &phraseDictionary = decodeStepList.front().GetPhraseDictionary();
  for (size_t startPos = 0 ; startPos < m_inputSentence.GetSize() ; startPos++)
    {
      // reuse phrase, add next word on
      Phrase sourcePhrase( m_inputSentence.GetDirection());

      for (size_t endPos = startPos ; endPos < m_inputSentence.GetSize() ; endPos++)
	{
	  const WordsRange wordsRange(startPos, endPos);

	  FactorArray &newWord = sourcePhrase.AddWord();
	  Word::Copy(newWord, m_inputSentence.GetFactorArray(endPos));

	  const TargetPhraseCollection *phraseColl =	phraseDictionary.FindEquivPhrase(sourcePhrase);
	  if (phraseColl != NULL)
	    {
	      if (verboseLevel >= 3) {
		cout << "[" << sourcePhrase << "; " << startPos << "-" << endPos << "]\n";
	      }
	      TargetPhraseCollection::const_iterator iterTargetPhrase;
	      for (iterTargetPhrase = phraseColl->begin() ; iterTargetPhrase != phraseColl->end() ; ++iterTargetPhrase)
		{
		  const TargetPhrase	&targetPhrase = *iterTargetPhrase;
					
		  const WordsRange wordsRange(startPos, endPos);
		  TranslationOption transOpt(wordsRange
					     , targetPhrase);

		  push_back(transOpt);
		  if (verboseLevel >= 3) {
		    cout << "\t" << transOpt << "\n";
		  }
		}
	      if (verboseLevel >= 3) { cout << endl; }
	    }
	  else if (sourcePhrase.GetSize() == 1)
	    {
	      // unknown word, add to target, and add as poss trans
	      //				float	weightWP		= m_staticData.GetWeightWordPenalty();
	      const FactorTypeSet &targetFactors 		= phraseDictionary.GetFactorsUsed(Output);
	      int isDigit = 0;
	      if (dropUnknown)
		{
		  const Factor *f = sourcePhrase.GetFactor(0, static_cast<FactorType>(0)); // surface @ 0
		  std::string s = f->ToString();
		  isDigit = s.find_first_of("0123456789");
		  if (isDigit == string::npos) isDigit = 0;
		  else isDigit = 1;
		  // modify the starting bitmap
		}
	      if (!dropUnknown || isDigit)
		{
		  // add to dictionary
		  TargetPhrase targetPhraseOrig(Output, &phraseDictionary);
		  FactorArray &targetWord = targetPhraseOrig.AddWord();
		  
		  const FactorArray &sourceWord = sourcePhrase.GetFactorArray(0);
		  
		  for (unsigned int currFactor = 0 ; currFactor < NUM_FACTORS ; currFactor++)
		    {
		      if (targetFactors.Contains(currFactor))
			{
			  FactorType factorType = static_cast<FactorType>(currFactor);
			  
			  const Factor *factor = sourceWord[factorType]
			    ,*unkownfactor;
			  switch (factorType)
			    {
			    case POS:
			      unkownfactor = factorCollection.AddFactor(Output, factorType, UNKNOWN_FACTOR);
			      targetWord[factorType] = unkownfactor;
			      break;
			    default:
			      unkownfactor = factorCollection.AddFactor(Output, factorType, factor->GetString());
			      targetWord[factorType] = unkownfactor;
			      break;
			    }
			}
		    }
		  
		  targetPhraseOrig.SetScore(allLM, weightWordPenalty);
		  
		  phraseDictionary.AddEquivPhrase(sourcePhrase, targetPhraseOrig);
		  const TargetPhraseCollection *phraseColl = phraseDictionary.FindEquivPhrase(sourcePhrase);
		  const TargetPhrase &targetPhrase = *phraseColl->begin();
		  
		  TranslationOption transOpt(wordsRange, targetPhrase);
		  
		  push_back(transOpt);
		}
	      else // drop source word
		{ m_initialCoverage.SetValue(startPos, startPos,1); }
	    }
	}
    }

  // create future score matrix
  // for each span in the source phrase (denoted by start and end)
  for(size_t startPos = 0; startPos < m_inputSentence.GetSize() ; startPos++) 
    {
      for(size_t endPos = startPos; endPos < m_inputSentence.GetSize() ; endPos++) 
	{
	  size_t length = endPos - startPos + 1;
	  vector< float > score(length + 1);
	  score[0] = 0;
	  for(size_t currLength = 1 ; currLength <= length ; currLength++) 
	    // initalize their future cost to -infinity
	    {
	      score[currLength] = - numeric_limits<float>::infinity();
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
	      cout<<"future cost from "<<startPos<<" to "<<endPos<<" is "<<score[length]<<endl;
	    }
	}
    }

}

