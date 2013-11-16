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

namespace Moses
{
using namespace std;

DecodeStepGeneration::DecodeStepGeneration(const GenerationDictionary* dict,
    const DecodeStep* prev,
    const std::vector<FeatureFunction*> &features)
  : DecodeStep(dict, prev, features)
{
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
  for (size_t currPos = 0 ; currPos < wordListVector.size() ; currPos++) {
    WordListIterator &iter = wordListIterVector[currPos];
    iter++;
    if (iter != wordListVector[currPos].end()) {
      // eg. 4 -> 5
      return;
    } else {
      //  eg 9 -> 10
      iter = wordListVector[currPos].begin();
    }
  }
}

void DecodeStepGeneration::Process(const TranslationOption &inputPartialTranslOpt
                                   , const DecodeStep &decodeStep
                                   , PartialTranslOptColl &outputPartialTranslOptColl
                                   , TranslationOptionCollection * /* toc */
                                   , bool /*adhereTableLimit*/) const
{
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0) {
    // word deletion

    TranslationOption *newTransOpt = new TranslationOption(inputPartialTranslOpt);
    outputPartialTranslOptColl.Add(newTransOpt);

    return;
  }

  // normal generation step
  const GenerationDictionary* generationDictionary  = decodeStep.GetGenerationDictionaryFeature();

  const Phrase &targetPhrase  = inputPartialTranslOpt.GetTargetPhrase();
  const InputPath &inputPath = inputPartialTranslOpt.GetInputPath();
  size_t targetLength         = targetPhrase.GetSize();

  // generation list for each word in phrase
  vector< WordList > wordListVector(targetLength);

  // create generation list
  int wordListVectorPos = 0;
  for (size_t currPos = 0 ; currPos < targetLength ; currPos++) { // going thorugh all words
    // generatable factors for this word to be put in wordList
    WordList &wordList = wordListVector[wordListVectorPos];
    const Word &word = targetPhrase.GetWord(currPos);

    // consult dictionary for possible generations for this word
    const OutputWordCollection *wordColl = generationDictionary->FindWord(word);

    if (wordColl == NULL) {
      // word not found in generation dictionary
      //toc->ProcessUnknownWord(sourceWordsRange.GetStartPos(), factorCollection);
      return; // can't be part of a phrase, special handling
    } else {
      // sort(*wordColl, CompareWordCollScore);
      OutputWordCollection::const_iterator iterWordColl;
      for (iterWordColl = wordColl->begin() ; iterWordColl != wordColl->end(); ++iterWordColl) {
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
  for (size_t currPos = 0 ; currPos < targetLength ; currPos++) {
    wordListIterVector[currPos] = wordListVector[currPos].begin();
    numIteration *= wordListVector[currPos].size();
  }

  // go thru each possible factor for each word & create hypothesis
  for (size_t currIter = 0 ; currIter < numIteration ; currIter++) {
    ScoreComponentCollection generationScore; // total score for this string of words

    // create vector of words with new factors for last phrase
    for (size_t currPos = 0 ; currPos < targetLength ; currPos++) {
      const WordPair &wordPair = *wordListIterVector[currPos];
      mergeWords[currPos] = &(wordPair.first);
      generationScore.PlusEquals(wordPair.second);
    }

    // merge with existing trans opt
    Phrase genPhrase( mergeWords);

    if (IsFilteringStep()) {
      if (!inputPartialTranslOpt.IsCompatible(genPhrase, m_conflictFactors))
        continue;
    }

    const TargetPhrase &inPhrase = inputPartialTranslOpt.GetTargetPhrase();
    TargetPhrase outPhrase(inPhrase);
    outPhrase.GetScoreBreakdown().PlusEquals(generationScore);

    outPhrase.MergeFactors(genPhrase, m_newOutputFactors);
    outPhrase.Evaluate(inputPath.GetPhrase(), m_featuresToApply);

    const WordsRange &sourceWordsRange = inputPartialTranslOpt.GetSourceWordsRange();

    TranslationOption *newTransOpt = new TranslationOption(sourceWordsRange, outPhrase);
    assert(newTransOpt);

    newTransOpt->SetInputPath(inputPath);

    outputPartialTranslOptColl.Add(newTransOpt);

    // increment iterators
    IncrementIterators(wordListIterVector, wordListVector);
  }
}

}


