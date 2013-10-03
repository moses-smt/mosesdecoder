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
  m_inputPathMatrix.resize(size);

  // 1-word phrases
  for (size_t startPos = 0; startPos < size; ++startPos) {
    vector<InputPathList> &vec = m_inputPathMatrix[startPos];
    vec.push_back(InputPathList());
    InputPathList &list = vec.back();

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

      list.push_back(path);

      m_phraseDictionaryQueue.push_back(path);
    }
  }

  // subphrases of 2+ words
  for (size_t phaseSize = 2; phaseSize <= size; ++phaseSize) {
    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
      size_t endPos = startPos + phaseSize -1;

      const std::vector<size_t> &nextNodes = input.GetNextNodes(endPos);

      WordsRange range(startPos, endPos);
      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

      vector<InputPathList> &vec = m_inputPathMatrix[startPos];
      vec.push_back(InputPathList());
      InputPathList &list = vec.back();

      // loop thru every previous path
      const InputPathList &prevPaths = GetInputPathList(startPos, endPos - 1);

      int prevNodesInd = 0;
      InputPathList::const_iterator iterPath;
      for (iterPath = prevPaths.begin(); iterPath != prevPaths.end(); ++iterPath) {
        //for (size_t pathInd = 0; pathInd < prevPaths.size(); ++pathInd) {
        const InputPath &prevPath = **iterPath;
        //const InputPath &prevPath = *prevPaths[pathInd];

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

          list.push_back(path);

          m_phraseDictionaryQueue.push_back(path);
        } // for (size_t i = 0; i < col.size(); ++i) {

        ++prevNodesInd;
      } // for (iterPath = prevPaths.begin(); iterPath != prevPaths.end(); ++iterPath) {
    }
  }
}

InputPathList &TranslationOptionCollectionLattice::GetInputPathList(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  CHECK(offset < m_inputPathMatrix[startPos].size());
  return m_inputPathMatrix[startPos][offset];
}

/* forcibly create translation option for a particular source word.
	* call the base class' ProcessOneUnknownWord() for each possible word in the confusion network
	* at a particular source position
*/
void TranslationOptionCollectionLattice::ProcessUnknownWord(size_t sourcePos)
{
  ConfusionNet const& source=dynamic_cast<ConfusionNet const&>(m_source);

  ConfusionNet::Column const& coll=source.GetColumn(sourcePos);
  const InputPathList &inputPathList = GetInputPathList(sourcePos, sourcePos);

  ConfusionNet::Column::const_iterator iterCol;
  InputPathList::const_iterator iterInputPath;
  size_t j=0;
  for(iterCol = coll.begin(), iterInputPath = inputPathList.begin();
      iterCol != coll.end();
      ++iterCol , ++iterInputPath) {
    const InputPath &inputPath = **iterInputPath;
    size_t length = source.GetColumnIncrement(sourcePos, j++);
    const ScorePair &inputScores = iterCol->second;
    ProcessOneUnknownWord(inputPath ,sourcePos, length, &inputScores);
  }

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
  CreateTranslationOptionsForRangeNew(decodeGraph, startPos, endPos, adhereTableLimit, graphInd);
}

void TranslationOptionCollectionLattice::CreateTranslationOptionsForRangeNew(
  const DecodeGraph &decodeGraph
  , size_t startPos
  , size_t endPos
  , bool adhereTableLimit
  , size_t graphInd)
{
  InputPathList &inputPathList = GetInputPathList(startPos, endPos);
  InputPathList::iterator iter;
  for (iter = inputPathList.begin(); iter != inputPathList.end(); ++iter) {
    InputPath &inputPath = **iter;
    TranslationOptionCollection::CreateTranslationOptionsForRange(decodeGraph
        , startPos
        , endPos
        , adhereTableLimit
        , graphInd
        , inputPath);

  }
}

} // namespace


