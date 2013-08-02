// $Id$

#include <list>
#include "TranslationOptionCollectionConfusionNet.h"
#include "ConfusionNet.h"
#include "DecodeStep.h"
#include "DecodeStepTranslation.h"
#include "FactorCollection.h"
#include "FF/InputFeature.h"
#include "TranslationModel/PhraseDictionaryTreeAdaptor.h"

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
  CHECK(inputFeature);

  size_t size = input.GetSize();
  m_targetPhrasesfromPt.resize(size);

  // 1-word phrases
  for (size_t startPos = 0; startPos < size; ++startPos) {
    vector<InputPathList> &vec = m_targetPhrasesfromPt[startPos];
    vec.push_back(InputPathList());
    InputPathList &list = vec.back();

    WordsRange range(startPos, startPos);
    const NonTerminalSet &labels = input.GetLabelSet(startPos, startPos);

    const ConfusionNet::Column &col = input.GetColumn(startPos);
    for (size_t i = 0; i < col.size(); ++i) {
      const Word &word = col[i].first;
      Phrase subphrase;
      subphrase.AddWord(word);

      const std::vector<float> &scores = col[i].second;
      ScoreComponentCollection *inputScore = new ScoreComponentCollection();
      inputScore->Assign(inputFeature, scores);

      InputPath *node = new InputPath(subphrase, labels, range, NULL, inputScore);
      list.push_back(node);

      m_phraseDictionaryQueue.push_back(node);
    }
  }

  // subphrases of 2+ words
  for (size_t phaseSize = 2; phaseSize <= size; ++phaseSize) {
    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
      size_t endPos = startPos + phaseSize -1;

      WordsRange range(startPos, endPos);
      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

      vector<InputPathList> &vec = m_targetPhrasesfromPt[startPos];
      vec.push_back(InputPathList());
      InputPathList &list = vec.back();

      // loop thru every previous path
      const InputPathList &prevNodes = GetInputPathList(startPos, endPos - 1);

      int prevNodesInd = 0;
      InputPathList::const_iterator iterPath;
      for (iterPath = prevNodes.begin(); iterPath != prevNodes.end(); ++iterPath) {
        //for (size_t pathInd = 0; pathInd < prevNodes.size(); ++pathInd) {
        const InputPath &prevNode = **iterPath;
        //const InputPath &prevNode = *prevNodes[pathInd];

        const Phrase &prevPhrase = prevNode.GetPhrase();
        const ScoreComponentCollection *prevInputScore = prevNode.GetInputScore();
        CHECK(prevInputScore);

        // loop thru every word at this position
        const ConfusionNet::Column &col = input.GetColumn(endPos);

        for (size_t i = 0; i < col.size(); ++i) {
          const Word &word = col[i].first;
          Phrase subphrase(prevPhrase);
          subphrase.AddWord(word);

          const std::vector<float> &scores = col[i].second;
          ScoreComponentCollection *inputScore = new ScoreComponentCollection(*prevInputScore);
          inputScore->PlusEquals(inputFeature, scores);

          InputPath *node = new InputPath(subphrase, labels, range, &prevNode, inputScore);
          list.push_back(node);

          m_phraseDictionaryQueue.push_back(node);
        } // for (size_t i = 0; i < col.size(); ++i) {

        ++prevNodesInd;
      } // for (iterPath = prevNodes.begin(); iterPath != prevNodes.end(); ++iterPath) {
    }
  }

  // check whether we should be using the old code to supportbinary phrase-table.
  // eventually, we'll stop support the binary phrase-table and delete this legacy code
  CheckLegacy();
}

InputPathList &TranslationOptionCollectionConfusionNet::GetInputPathList(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  CHECK(offset < m_targetPhrasesfromPt[startPos].size());
  return m_targetPhrasesfromPt[startPos][offset];
}

/* forcibly create translation option for a particular source word.
	* call the base class' ProcessOneUnknownWord() for each possible word in the confusion network
	* at a particular source position
*/
void TranslationOptionCollectionConfusionNet::ProcessUnknownWord(size_t sourcePos)
{
  ConfusionNet const& source=dynamic_cast<ConfusionNet const&>(m_source);

  ConfusionNet::Column const& coll=source.GetColumn(sourcePos);
  size_t j=0;
  for(ConfusionNet::Column::const_iterator i=coll.begin(); i!=coll.end(); ++i) {
    ProcessOneUnknownWord(i->first ,sourcePos, source.GetColumnIncrement(sourcePos, j++),&(i->second));
  }

}

void TranslationOptionCollectionConfusionNet::CreateTranslationOptions()
{
  if (!m_useLegacy) {
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
  if (m_useLegacy) {
    CreateTranslationOptionsForRangeLegacy(decodeGraph, startPos, endPos, adhereTableLimit, graphInd);
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

void TranslationOptionCollectionConfusionNet::CreateTranslationOptionsForRangeLegacy(
  const DecodeGraph &decodeGraph
  , size_t startPos
  , size_t endPos
  , bool adhereTableLimit
  , size_t graphInd)
{
  if ((StaticData::Instance().GetXmlInputType() != XmlExclusive) || !HasXmlOptionsOverlappingRange(startPos,endPos)) {
    Phrase *sourcePhrase = NULL; // can't initialise with substring, in case it's confusion network

    // partial trans opt stored in here
    PartialTranslOptColl* oldPtoc = new PartialTranslOptColl;
    size_t totalEarlyPruned = 0;

    // initial translation step
    list <const DecodeStep* >::const_iterator iterStep = decodeGraph.begin();
    const DecodeStep &decodeStep = **iterStep;

    static_cast<const DecodeStepTranslation&>(decodeStep).ProcessInitialTranslation
    (m_source, *oldPtoc
     , startPos, endPos, adhereTableLimit );

    // do rest of decode steps
    int indexStep = 0;

    for (++iterStep ; iterStep != decodeGraph.end() ; ++iterStep) {

      const DecodeStep &decodeStep = **iterStep;
      PartialTranslOptColl* newPtoc = new PartialTranslOptColl;

      // go thru each intermediate trans opt just created
      const vector<TranslationOption*>& partTransOptList = oldPtoc->GetList();
      vector<TranslationOption*>::const_iterator iterPartialTranslOpt;
      for (iterPartialTranslOpt = partTransOptList.begin() ; iterPartialTranslOpt != partTransOptList.end() ; ++iterPartialTranslOpt) {
        TranslationOption &inputPartialTranslOpt = **iterPartialTranslOpt;

        decodeStep.Process(inputPartialTranslOpt
                           , decodeStep
                           , *newPtoc
                           , this
                           , adhereTableLimit
                           , *sourcePhrase);
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

void TranslationOptionCollectionConfusionNet::CheckLegacy()
{
  const std::vector<PhraseDictionary*> &pts = StaticData::Instance().GetPhraseDictionaries();
  for (size_t i = 0; i < pts.size(); ++i) {
    const PhraseDictionary *phraseDictionary = pts[i];
    if (dynamic_cast<const PhraseDictionaryTreeAdaptor*>(phraseDictionary) != NULL) {
      m_useLegacy = true;
      return;
    }
  }

  m_useLegacy = false;
}

}


