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

      m_phraseDictionaryQueue.push_back(path);
    }
  }

  // subphrases of 2+ words
  for (size_t phaseSize = 2; phaseSize <= size; ++phaseSize) {
    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
      size_t endPos = startPos + phaseSize -1;

      const std::vector<size_t> &nextNodes = input.GetNextNodes(endPos);
      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

      // loop thru every previous paths
      size_t numPrevPaths = m_phraseDictionaryQueue.size();

      for (size_t i = 0; i < numPrevPaths; ++i) {
        //for (size_t pathInd = 0; pathInd < prevPaths.size(); ++pathInd) {
        const InputPath &prevPath = *m_phraseDictionaryQueue[i];

        size_t nextNode = prevPath.GetNextNode();
        if (prevPath.GetWordsRange().GetEndPos() + nextNode != endPos) {
        	continue;
        }

        WordsRange range(prevPath.GetWordsRange().GetStartPos(), endPos);

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

          cerr << *path << endl;

          m_phraseDictionaryQueue.push_back(path);
        } // for (size_t i = 0; i < col.size(); ++i) {

      } // for (size_t i = 0; i < numPrevPaths; ++i) {
    }
  }


  // debug
  for (size_t i = 0; i < m_phraseDictionaryQueue.size(); ++i) {
	  const InputPath &prevPath = *m_phraseDictionaryQueue[i];
	  cerr << prevPath << endl;
  }
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
  if (!StaticData::Instance().GetUseLegacyPT()) {
    GetTargetPhraseCollectionBatch();
  }
  TranslationOptionCollection::CreateTranslationOptions();
}


/** create translation options that exactly cover a specific input span.
 * Called by CreateTranslationOptions() and ProcessUnknownWord()
 * \param decodeGraph list of decoding steps
 * \param factorCollection input sentence with all factors
 * \param startPos first position in input sentence
 * \param lastPos last position in input sentence
 * \param adhereTableLimit whether phrase & generation table limits are adhered to
 */
void TranslationOptionCollectionLattice::CreateTranslationOptionsForRange(
  const DecodeGraph &decodeGraph
  , size_t startPos
  , size_t endPos
  , bool adhereTableLimit
  , size_t graphInd)
{
  //CreateTranslationOptionsForRangeNew(decodeGraph, startPos, endPos, adhereTableLimit, graphInd);
}

} // namespace


