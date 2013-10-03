// $Id$

#include <list>
#include "TranslationOptionCollectionLattice.h"
#include "ConfusionNet.h"
#include "WordLattice.h"
#include "DecodeGraph.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "FactorCollection.h"
#include "FF/InputFeature.h"
#include "TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "util/check.hh"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

/** constructor; just initialize the base class */
TranslationOptionCollectionLattice::TranslationOptionCollectionLattice(
  const WordLattice &input
  , size_t maxNoTransOptPerCoverage, float translationOptionThreshold)
  : TranslationOptionCollection(input, maxNoTransOptPerCoverage, translationOptionThreshold)
{
  CHECK(!StaticData::Instance().GetUseLegacyPT());

  const InputFeature *inputFeature = StaticData::Instance().GetInputFeature();
  CHECK(inputFeature);

  size_t size = input.GetSize();

  // 1-word phrases
  for (size_t startPos = 0; startPos < size; ++startPos) {

    const std::vector<size_t> &nextNodes = input.GetNextNodes(startPos);

    WordsRange range(startPos, startPos);
    const NonTerminalSet &labels = input.GetLabelSet(startPos, startPos);

    const ConfusionNet::Column &col = input.GetColumn(startPos);
    for (size_t i = 0; i < col.size(); ++i) {
      const Word &word = col[i].first;
      Phrase subphrase;
      subphrase.AddWord(word);

      const ScorePair &scores = col[i].second;
      ScorePair *inputScore = new ScorePair(scores);

      InputPath *path = new InputPath(subphrase, labels, range, NULL, inputScore);

      size_t nextNode = nextNodes[i];
      path->SetNextNode(nextNode);

      m_inputPathQueue.push_back(path);
    }
  }

  // iteratively extend all paths
    for (size_t endPos = 1; endPos < size; ++endPos) {
      const std::vector<size_t> &nextNodes = input.GetNextNodes(endPos);

      // loop thru every previous paths
      size_t numPrevPaths = m_inputPathQueue.size();

      for (size_t i = 0; i < numPrevPaths; ++i) {
        //for (size_t pathInd = 0; pathInd < prevPaths.size(); ++pathInd) {
        const InputPath &prevPath = *m_inputPathQueue[i];

        size_t nextNode = prevPath.GetNextNode();
        if (prevPath.GetWordsRange().GetEndPos() + nextNode != endPos) {
        	continue;
        }

        size_t startPos = prevPath.GetWordsRange().GetStartPos();
        WordsRange range(startPos, endPos);
        const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

        const Phrase &prevPhrase = prevPath.GetPhrase();
        const ScorePair *prevInputScore = prevPath.GetInputScore();
        CHECK(prevInputScore);

        // loop thru every word at this position
        const ConfusionNet::Column &col = input.GetColumn(endPos);

        for (size_t i = 0; i < col.size(); ++i) {
          const Word &word = col[i].first;
          Phrase subphrase(prevPhrase);
          subphrase.AddWord(word);

          const ScorePair &scores = col[i].second;
          ScorePair *inputScore = new ScorePair(*prevInputScore);
          inputScore->PlusEquals(scores);

          InputPath *path = new InputPath(subphrase, labels, range, &prevPath, inputScore);

          size_t nextNode = nextNodes[i];
          path->SetNextNode(nextNode);

          m_inputPathQueue.push_back(path);
        } // for (size_t i = 0; i < col.size(); ++i) {

      } // for (size_t i = 0; i < numPrevPaths; ++i) {
    }
}

void TranslationOptionCollectionLattice::ProcessUnknownWord()
{

}

/* forcibly create translation option for a particular source word.
	* call the base class' ProcessOneUnknownWord() for each possible word in the confusion network
	* at a particular source position
*/
void TranslationOptionCollectionLattice::ProcessUnknownWord(size_t sourcePos)
{

}

void TranslationOptionCollectionLattice::CreateTranslationOptions()
{
  GetTargetPhraseCollectionBatch();
  //TranslationOptionCollection::CreateTranslationOptions();

  VERBOSE(2,"Translation Option Collection\n " << *this << endl);

  ProcessUnknownWord();

  EvaluateWithSource();

  // Prune
  Prune();

  Sort();

  // future score matrix
  CalcFutureScore();

  // Cached lex reodering costs
  CacheLexReordering();

}

void TranslationOptionCollectionLattice::CreateTranslationOptionsForRange(const DecodeGraph &decodeStepList
      , size_t startPosition
      , size_t endPosition
      , bool adhereTableLimit
      , size_t graphInd)
{
	UTIL_THROW(util::Exception, "CreateTranslationOptionsForRange() not implemented for lattice");
}

} // namespace


