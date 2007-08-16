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

#include "DecodeStepGeneration.h"
#include "GenerationDictionary.h"
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "PartialTranslOptColl.h"
#include "FactorCollection.h"

//variables that help is keeping the partial and total scores of each generation list combination 
float outputStringScore, sumUp =0;

DecodeStepGeneration::DecodeStepGeneration(GenerationDictionary* dict, const DecodeStep* prev)
: DecodeStep(dict, prev)
{
}

const GenerationDictionary &DecodeStepGeneration::GetGenerationDictionary() const
{
  return *static_cast<const GenerationDictionary*>(m_ptr);
}

TranslationOption *DecodeStepGeneration::MergeGeneration(const TranslationOption& oldTO, Phrase &mergePhrase
                                  , const ScoreComponentCollection& generationScore) const
{
	if (IsFilteringStep()) {
  	if (!oldTO.IsCompatible(mergePhrase, m_conflictFactors)) 
			return NULL;
	}

  TranslationOption *newTransOpt = new TranslationOption(oldTO);
  newTransOpt->MergeNewFeatures(mergePhrase, generationScore, m_newOutputFactors);
  return newTransOpt;
}

// helpers
typedef pair<Word, ScoreComponentCollection> WordPair;
typedef list< WordPair > WordList;
// 1st = word
// 2nd = score
typedef list< WordPair >::const_iterator WordListIterator;

/** used in generation: increases iterators when looping through the exponential number of generation expansions */
inline void IncrementIterators(vector< WordListIterator > &wordListIterVector
                               , const vector< WordList > &wordListVector)
{
  for (size_t currPos = 0 ; currPos < wordListVector.size() ; currPos++)
    {
      WordListIterator &iter = wordListIterVector[currPos];
      iter++;
      if (iter != wordListVector[currPos].end())
        { // eg. 4 -> 5
          return;
        }
      else
        { //  eg 9 -> 10
          iter = wordListVector[currPos].begin();
        }
    }
}

//This class helps sort the generation list 
 class SortGenList 
  {
	  size_t m_targetLength;
	  size_t targetLength;
	  const Word *m_outputWord;
	  float m_totalScore;
	  public:
		SortGenList (const Word *outputWord, float totalScore)
		{
				
			m_totalScore = totalScore;
			m_outputWord = outputWord;
			m_targetLength = targetLength;

		}
		float getScore()
		{
		 return m_totalScore;
		}

        const Word getOutputWord()
		{
		  return *m_outputWord;
		}
		size_t getTargetLength()
		{
			return m_targetLength;
		}
    
 };

 //Implements a simple sort algorithm that helps us sort the factors according to descending order of scores
 
 vector<SortGenList> b_sort(vector <SortGenList> &array)
{
      int i, j, flag = 1;    // set flag to 1 to begin initial pass
      //int temp;             // holding variable
      int arrayLength = array.size( ); 
      for(i = 1; (i <= arrayLength) && flag; i++)
     {
          flag = 0;
          for (j=0; j < (arrayLength -1); j++)
         {
               if (array[j+1].getScore() > array[j].getScore())      // ascending order simply changes to <
              { 
                    SortGenList temp = array[j];             // swap elements
                    array[j] = array[j+1];
                    array[j+1] = temp;
                    flag = 1;               // indicates that a swap occurred.
               }
          }
     }
     return array;   //arrays are passed to functions by address; nothing is returned
}



// Gets the best one, two or three from the sorted genlist as the length of possible choices permits
vector <SortGenList> topThree(vector <SortGenList> toSelect)
{
	vector<SortGenList>::iterator iter;
	vector <SortGenList> best;
	if(toSelect.size()==1)
	{
		best.push_back(toSelect[0]);
		return best;
	}
	else if (toSelect.size()==2)
	{
		best.push_back(toSelect[0]);
		best.push_back(toSelect[1]);
		return best;
	}
	else
	{
		for( iter = toSelect.begin(); iter != toSelect.end(); iter++ )
		{
			best.push_back(toSelect[0]);
			best.push_back(toSelect[1]);
			best.push_back(toSelect[2]);
			return best;
		}
	}
	
	
}


void DecodeStepGeneration::Process(const TranslationOption &inputPartialTranslOpt
                              , const DecodeStep &decodeStep
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , TranslationOptionCollection *toc
                              , bool adhereTableLimit) const
{
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
    { // word deletion

      TranslationOption *newTransOpt = new TranslationOption(inputPartialTranslOpt);
      outputPartialTranslOptColl.Add(newTransOpt);

      return;
    }


  
  // normal generation step
  const GenerationDictionary &generationDictionary  = decodeStep.GetGenerationDictionary();
//  const WordsRange &sourceWordsRange                = inputPartialTranslOpt.GetSourceWordsRange();

  const Phrase &targetPhrase  = inputPartialTranslOpt.GetTargetPhrase();
  size_t targetLength         = targetPhrase.GetSize();

  // generation list for each word in phrase
  vector< WordList > wordListVector(targetLength);

  // lists of classes for generation list
  vector <SortGenList> genList;	

  //holds the sorted generation list 
  vector <SortGenList> sortedGenList;

  //holds the top3 members of the generation list
  vector <SortGenList> best3;

  //iterators for SortGenList Class
  vector<SortGenList>::iterator sortGenListIterVector;
  vector<SortGenList>::iterator iter;

  
  // create generation list
  int wordListVectorPos = 0;
  for (size_t currPos = 0 ; currPos < targetLength ; currPos++) // going thorugh all words
    {
      // generatable factors for this word to be put in wordList
      WordList &wordList = wordListVector[wordListVectorPos];
      const Word &word = targetPhrase.GetWord(currPos);

      // consult dictionary for possible generations for this word
      const OutputWordCollection *wordColl = generationDictionary.FindWord(word);

      
	  if (wordColl == NULL)
        { // word not found in generation dictionary
          //toc->ProcessUnknownWord(sourceWordsRange.GetStartPos(), factorCollection);
          return; // can't be part of a phrase, special handling
        }
      else
        {
          // sort(*wordColl, CompareWordCollScore);
          OutputWordCollection::const_iterator iterWordColl;
          for (iterWordColl = wordColl->begin() ; iterWordColl != wordColl->end(); ++iterWordColl)
            {
              const Word &outputWord = (*iterWordColl).first;
              const ScoreComponentCollection& score = (*iterWordColl).second;
			  float totalScore = score.InnerProduct(StaticData::Instance().GetAllWeights());

              // enter into word list generated factor(s) and its(their) score(s)
              wordList.push_back(WordPair(outputWord, score));
			  			  
			  //Creates a list of possible factors for a word together with their score and stores them in genList 			   
			  SortGenList list(&outputWord, totalScore);			  
			  genList.push_back(list);  			 
			  	
            }
		  
		  //Sort the list of factors according to descending order of scores before moving to the next word
		  sortedGenList = b_sort(genList);

		  //Select the top three from the sorted version
		  best3 = topThree(sortedGenList);
		  
		  //print sorted list for debug purposes
		  vector<SortGenList>::iterator iter;
		  for( iter = genList.begin(); iter != genList.end(); iter++ )
		  {
			  SortGenList &list = *iter;
			  cout<< "my choices here" << endl;
			  cout << list.getOutputWord() << endl;
			  cout << list.getScore() << endl;
			  
		  }

		  //print the best three for debug purposes		  
		  for( iter = best3.begin(); iter != best3.end(); iter++ )
		  {
			  SortGenList &list = *iter;
			  cout<< " so my best here is" << endl;
			  cout << list.getOutputWord() << endl;
			  cout << list.getScore() << endl;	
		  } 
		  
		  outputStringScore= best3[0].getScore();
		  sumUp = sumUp + outputStringScore;
          	  
		  wordListVectorPos++; // done, next word
		  		 	  		 
        }
  
   //code's mine to here
    }
   cout<<" summing here"<<sumUp<<endl;

  // use generation list (wordList)
  // set up iterators (total number of expansions)
  size_t numIteration = 1;
  vector< WordListIterator >  wordListIterVector(targetLength);
  vector< const Word* >       mergeWords(targetLength);
  for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
    {
      wordListIterVector[currPos] = wordListVector[currPos].begin();
      numIteration *= wordListVector[currPos].size();
    }

  // go thru each possible factor for each word & create hypothesis
  for (size_t currIter = 0 ; currIter < numIteration ; currIter++)
    {
      ScoreComponentCollection generationScore; // total score for this string of words

      // create vector of words with new factors for last phrase
      for (size_t currPos = 0 ; currPos < targetLength ; currPos++)
        {
          const WordPair &wordPair = *wordListIterVector[currPos];
          mergeWords[currPos] = &(wordPair.first);
          generationScore.PlusEquals(wordPair.second);
        }

      // merge with existing trans opt
      Phrase genPhrase(Output, mergeWords);
      TranslationOption *newTransOpt = MergeGeneration(inputPartialTranslOpt, genPhrase, generationScore);
      if (newTransOpt != NULL)
        {
          outputPartialTranslOptColl.Add(newTransOpt);
        }

      // increment iterators
      IncrementIterators(wordListIterVector, wordListVector);
    }

}

 
