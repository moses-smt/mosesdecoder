#include "DecodeStep_Generation.h"
#include "GenerationDictionary.h"
#include "TranslationOption.h"
#include "TranslationOptionCollection.h"
#include "PartialTranslOptColl.h"
#include "FactorCollection.h"

GenerationDecodeStep::GenerationDecodeStep(GenerationDictionary* dict, const DecodeStep* prev)
: DecodeStep(dict, prev)
{
}

const GenerationDictionary &GenerationDecodeStep::GetGenerationDictionary() const
{
  return *static_cast<const GenerationDictionary*>(m_ptr);
}

TranslationOption *GenerationDecodeStep::MergeGeneration(const TranslationOption& oldTO, Phrase &mergePhrase
                                  , const ScoreComponentCollection2& generationScore) const
{
	if (IsFilteringStep()) {
  	if (!oldTO.IsCompatible(mergePhrase, m_conflictFactors)) return 0;
	}

  TranslationOption *newTransOpt = new TranslationOption(oldTO);
  newTransOpt->MergeNewFeatures(mergePhrase, generationScore, m_newOutputFactors);
  return newTransOpt;
}

// helpers
typedef pair<Word, ScoreComponentCollection2> WordPair;
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

void GenerationDecodeStep::Process(const TranslationOption &inputPartialTranslOpt
                              , const DecodeStep &decodeStep
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , FactorCollection &factorCollection
                              , TranslationOptionCollection *toc) const
{
  //TRACE_ERR(inputPartialTranslOpt << endl);
  if (inputPartialTranslOpt.GetTargetPhrase().GetSize() == 0)
    { // word deletion

      TranslationOption *newTransOpt = new TranslationOption(inputPartialTranslOpt);
      outputPartialTranslOptColl.Add(newTransOpt);

      return;
    }

  // normal generation step
  const GenerationDictionary &generationDictionary  = decodeStep.GetGenerationDictionary();
  const WordsRange &sourceWordsRange                = inputPartialTranslOpt.GetSourceWordsRange();

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
      const FactorArray &factorArray = targetPhrase.GetFactorArray(currPos);

      // consult dictionary for possible generations for this word
      const OutputWordCollection *wordColl = generationDictionary.FindWord(factorArray);

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
              const ScoreComponentCollection2& score = (*iterWordColl).second;
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
      ScoreComponentCollection2 generationScore; // total score for this string of words

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

