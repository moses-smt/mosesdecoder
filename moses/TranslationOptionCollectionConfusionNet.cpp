// $Id$

#include <list>
#include "TranslationOptionCollectionConfusionNet.h"
#include "ConfusionNet.h"
#include "DecodeGraph.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "FactorCollection.h"
#include "FF/InputFeature.h"
#include "TranslationModel/PhraseDictionaryTreeAdaptor.h"
#include "util/exception.hh"

using namespace std;

namespace Moses
{

/** constructor; just initialize the base class */
TranslationOptionCollectionConfusionNet::TranslationOptionCollectionConfusionNet(
  const ConfusionNet &input
  , size_t maxNoTransOptPerCoverage, float translationOptionThreshold)
  : TranslationOptionCollection(input, maxNoTransOptPerCoverage, translationOptionThreshold)
{
  const InputFeature *inputFeature = StaticData::Instance().GetInputFeature();
  UTIL_THROW_IF2(inputFeature == NULL,
		  "Input feature must be specified");

  size_t inputSize = input.GetSize();
  m_inputPathMatrix.resize(inputSize);

  size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
  maxSizePhrase = std::min(inputSize, maxSizePhrase);

  // 1-word phrases
  for (size_t startPos = 0; startPos < inputSize; ++startPos) {
    vector<InputPathList> &vec = m_inputPathMatrix[startPos];
    vec.push_back(InputPathList());
    InputPathList &list = vec.back();

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
      list.push_back(path);

      m_inputPathQueue.push_back(path);
    }
  }

  // subphrases of 2+ words
  for (size_t phraseSize = 2; phraseSize <= maxSizePhrase; ++phraseSize) {
    for (size_t startPos = 0; startPos < inputSize - phraseSize + 1; ++startPos) {
      size_t endPos = startPos + phraseSize -1;

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
        UTIL_THROW_IF2(prevInputScore == NULL,
        		"No input score for path: " << prevPath);

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
          list.push_back(path);

          m_inputPathQueue.push_back(path);
        } // for (size_t i = 0; i < col.size(); ++i) {

        ++prevNodesInd;
      } // for (iterPath = prevPaths.begin(); iterPath != prevPaths.end(); ++iterPath) {
    }
  }
}

InputPathList &TranslationOptionCollectionConfusionNet::GetInputPathList(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  UTIL_THROW_IF2(offset >= m_inputPathMatrix[startPos].size(),
                 "Out of bound access: " << offset);

  return m_inputPathMatrix[startPos][offset];
}

/* forcibly create translation option for a particular source word.
	* call the base class' ProcessOneUnknownWord() for each possible word in the confusion network
	* at a particular source position
*/
void TranslationOptionCollectionConfusionNet::ProcessUnknownWord(size_t sourcePos)
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

void TranslationOptionCollectionConfusionNet::CreateTranslationOptions()
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
void TranslationOptionCollectionConfusionNet::CreateTranslationOptionsForRange(
  const DecodeGraph &decodeGraph
  , size_t startPos
  , size_t endPos
  , bool adhereTableLimit
  , size_t graphInd)
{
  if (StaticData::Instance().GetUseLegacyPT()) {
    CreateTranslationOptionsForRangeLEGACY(decodeGraph, startPos, endPos, adhereTableLimit, graphInd);
  } else {
    CreateTranslationOptionsForRangeNew(decodeGraph, startPos, endPos, adhereTableLimit, graphInd);
  }
}

void TranslationOptionCollectionConfusionNet::CreateTranslationOptionsForRangeNew(
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

void TranslationOptionCollectionConfusionNet::CreateTranslationOptionsForRangeLEGACY(
  const DecodeGraph &decodeGraph
  , size_t startPos
  , size_t endPos
  , bool adhereTableLimit
  , size_t graphInd)
{
  if ((StaticData::Instance().GetXmlInputType() != XmlExclusive) || !HasXmlOptionsOverlappingRange(startPos,endPos)) {
    InputPathList &inputPathList = GetInputPathList(startPos, endPos);

    // partial trans opt stored in here
    PartialTranslOptColl* oldPtoc = new PartialTranslOptColl;
    size_t totalEarlyPruned = 0;

    // initial translation step
    list <const DecodeStep* >::const_iterator iterStep = decodeGraph.begin();
    const DecodeStep &decodeStep = **iterStep;

    static_cast<const DecodeStepTranslation&>(decodeStep).ProcessInitialTranslationLEGACY
    (m_source, *oldPtoc
     , startPos, endPos, adhereTableLimit, inputPathList );

    // do rest of decode steps
    int indexStep = 0;

    for (++iterStep ; iterStep != decodeGraph.end() ; ++iterStep) {

      const DecodeStep *decodeStep = *iterStep;
      const DecodeStepTranslation *transStep =dynamic_cast<const DecodeStepTranslation*>(decodeStep);
      const DecodeStepGeneration *genStep =dynamic_cast<const DecodeStepGeneration*>(decodeStep);

      PartialTranslOptColl* newPtoc = new PartialTranslOptColl;

      // go thru each intermediate trans opt just created
      const vector<TranslationOption*>& partTransOptList = oldPtoc->GetList();
      vector<TranslationOption*>::const_iterator iterPartialTranslOpt;
      for (iterPartialTranslOpt = partTransOptList.begin() ; iterPartialTranslOpt != partTransOptList.end() ; ++iterPartialTranslOpt) {
        TranslationOption &inputPartialTranslOpt = **iterPartialTranslOpt;

        if (transStep) {
          transStep->ProcessLEGACY(inputPartialTranslOpt
                                   , *decodeStep
                                   , *newPtoc
                                   , this
                                   , adhereTableLimit);
        } else {
          assert(genStep);
          genStep->Process(inputPartialTranslOpt
                           , *decodeStep
                           , *newPtoc
                           , this
                           , adhereTableLimit);
        }
      }

      // last but 1 partial trans not required anymore
      totalEarlyPruned += newPtoc->GetPrunedCount();
      delete oldPtoc;
      oldPtoc = newPtoc;

      indexStep++;
    } // for (++iterStep

    // add to fully formed translation option list
    PartialTranslOptColl &lastPartialTranslOptColl	= *oldPtoc;
    const vector<TranslationOption*>& partTransOptList = lastPartialTranslOptColl.GetList();
    vector<TranslationOption*>::const_iterator iterColl;
    for (iterColl = partTransOptList.begin() ; iterColl != partTransOptList.end() ; ++iterColl) {
      TranslationOption *transOpt = *iterColl;
      Add(transOpt);
    }

    lastPartialTranslOptColl.DetachAll();
    totalEarlyPruned += oldPtoc->GetPrunedCount();
    delete oldPtoc;
    // TRACE_ERR( "Early translation options pruned: " << totalEarlyPruned << endl);

  } // if ((StaticData::Instance().GetXmlInputType() != XmlExclusive) || !HasXmlOptionsOverlappingRange(startPos,endPos))

  if (graphInd == 0 && StaticData::Instance().GetXmlInputType() != XmlPassThrough && HasXmlOptionsOverlappingRange(startPos,endPos)) {
    CreateXmlOptionsForRange(startPos, endPos);
  }
}


}


