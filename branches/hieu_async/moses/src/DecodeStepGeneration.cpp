// $Id: DecodeStepGeneration.cpp 141 2007-10-12 00:25:31Z hieu $

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

DecodeStepGeneration::DecodeStepGeneration()
{}

const GenerationDictionary &DecodeStepGeneration::GetGenerationDictionary() const
{
  return *static_cast<const GenerationDictionary*>(m_ptr);
}

bool DecodeStepGeneration::Load(const std::string &filePath
															, size_t numFeatures
															, const std::vector<FactorType> &input
															, const std::vector<FactorType> &output
															, ScoreIndexManager	&scoreIndexManager)
{
	GenerationDictionary *genDict = new GenerationDictionary(numFeatures, scoreIndexManager);
	assert(genDict && "could not create GenerationDictionary");
	if (!genDict->Load(input
									, output
									, filePath
									, Output				// always target, should we allow source?
									))
	{
		delete genDict;
		return false;
	}

	SetDictionary(genDict);
	return true;
}

TranslationOption *DecodeStepGeneration::MergeGeneration(const TranslationOption& oldTO, Phrase &mergePhrase
                                  , const ScoreComponentCollection& generationScore) const
{
  TranslationOption *newTransOpt = new TranslationOption(oldTO);
	const FactorMask &factorMask = GetOutputFactorMask();
	std::vector<FactorType> featuresToMerge;

	for (size_t factorType = 0 ; factorType < 2 ; factorType++)
	{
		if (factorMask[factorType])
			featuresToMerge.push_back(factorType);
	}
	newTransOpt->MergeNewFeatures(mergePhrase, generationScore, featuresToMerge);
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

void DecodeStepGeneration::Process(const TranslationOption &inputPartialTranslOpt
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , bool adhereTableLimit) const
{
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
    { // word deletion

      TranslationOption *newTransOpt = new TranslationOption(inputPartialTranslOpt);
      outputPartialTranslOptColl.Add(newTransOpt);

      return;
    }

  // normal generation step
  const GenerationDictionary &generationDictionary  = GetGenerationDictionary();

  const Phrase &targetPhrase  = inputPartialTranslOpt.GetTargetPhrase();
  size_t targetLength         = targetPhrase.GetSize();

  // generation list for each word in phrase
  vector< WordList > wordListVector(targetLength);

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
              // enter into word list generated factor(s) and its(their) score(s)
              wordList.push_back(WordPair(outputWord, score));
            }

          wordListVectorPos++; // done, next word
        }
    }

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

