// $Id$

#include <cassert>
#include <iostream>
#include "TranslationOptionCollectionConfusionNet.h"
#include "DecodeStep.h"
#include "FactorCollection.h"
#include "DecodeStepTranslation.h"
#include "DecodeStepGeneration.h"
#include "moses/FF/InputFeature.h"

using namespace std;

namespace Moses
{

/** constructor; just initialize the base class */
TranslationOptionCollectionConfusionNet::TranslationOptionCollectionConfusionNet(
  const ConfusionNet &input
  , size_t maxNoTransOptPerCoverage, float translationOptionThreshold)
  : TranslationOptionCollection(input, maxNoTransOptPerCoverage, translationOptionThreshold)
{
  const StaticData &staticData = StaticData::Instance();
  const InputFeature *inputFeature = staticData.GetInputFeature();
  CHECK(inputFeature);

  size_t size = input.GetSize();

  // create matrix
  for (size_t startPos = 0; startPos < size; ++startPos) {
    std::vector<std::vector<SourcePath> > vec;
    m_collection.push_back( vec );
    size_t maxSize = size - startPos;
    size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
    maxSize = std::min(maxSize, maxSizePhrase);

    for (size_t endPos = 0 ; endPos < maxSize ; ++endPos) {
      std::vector<SourcePath> vec;
      m_collection[startPos].push_back( vec );
    }


    // cut up confusion network into substrings
    // start with 1-word phrases
    std::vector<SourcePath> &subphrases = GetPhrases(startPos, startPos);
    assert(subphrases.size() == 0);

    const ConfusionNet::Column &col = input.GetColumn(startPos);
    ConfusionNet::Column::const_iterator iter;
    for (iter = col.begin(); iter != col.end(); ++iter) {
      subphrases.push_back(SourcePath());
      SourcePath &sourcePath = subphrases.back();

      const std::pair<Word,std::vector<float> > &inputNode = *iter;

      //cerr << "word=" << inputNode.first << " scores=" << inputNode.second.size() << endl;
      sourcePath.first.AddWord(inputNode.first);
      sourcePath.second.PlusEquals(inputFeature, inputNode.second);

    } // for (iter = col.begin(); iter != col.end(); ++iter) {
  } // for (size_t startPos = 0; startPos < size; ++startPos) {

  // create subphrases by appending words to previous subphrases
  for (size_t startPos = 0; startPos < size; ++startPos) {
	size_t maxSize = size - startPos;
	size_t maxSizePhrase = StaticData::Instance().GetMaxPhraseLength();
	maxSize = std::min(maxSize, maxSizePhrase);
	size_t end = startPos + maxSize - 1;

    for (size_t endPos = startPos + 1; endPos < end; ++endPos) {
      std::vector<SourcePath> &newSubphrases = GetPhrases(startPos, endPos);
      const std::vector<SourcePath> &prevSubphrases = GetPhrases(startPos, endPos - 1);
      const ConfusionNet::Column &col = input.GetColumn(endPos);
      CreateSubPhrases(newSubphrases, prevSubphrases, col, *inputFeature);
    }
  }

  /*
  for (size_t startPos = 0; startPos < size; ++startPos) {
    for (size_t endPos = startPos; endPos < size; ++endPos) {
    	cerr << "RANGE=" << startPos << "-" << endPos << endl;

    	const std::vector<SourcePath> &subphrases = GetPhrases(startPos, endPos);
    	std::vector<SourcePath>::const_iterator iterSourcePath;
    	for (iterSourcePath = subphrases.begin(); iterSourcePath != subphrases.end(); ++iterSourcePath) {
    		const SourcePath &sourcePath = *iterSourcePath;
    		cerr << sourcePath.first << " " <<sourcePath.second << endl;
    	}
    }
  }
  */
}

void TranslationOptionCollectionConfusionNet::CreateSubPhrases(std::vector<SourcePath> &newSubphrases
    , const std::vector<SourcePath> &prevSubphrases
    , const ConfusionNet::Column &col
    , const InputFeature &inputFeature)
{
  std::vector<SourcePath>::const_iterator iterSourcePath;
  for (iterSourcePath = prevSubphrases.begin(); iterSourcePath != prevSubphrases.end(); ++iterSourcePath) {
    const SourcePath &sourcePath = *iterSourcePath;
    const Phrase &prevSubPhrase = sourcePath.first;
    const ScoreComponentCollection &prevScore = sourcePath.second;

    ConfusionNet::Column::const_iterator iterCol;
    for (iterCol = col.begin(); iterCol != col.end(); ++iterCol) {
      const std::pair<Word,std::vector<float> > &node = *iterCol;
      Phrase subphrase(prevSubPhrase);
      subphrase.AddWord(node.first);

      ScoreComponentCollection score(prevScore);
      score.PlusEquals(&inputFeature, node.second);

      SourcePath newSourcePath(subphrase, score);
      newSubphrases.push_back(newSourcePath);
    }
  }
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

const std::vector<TranslationOptionCollectionConfusionNet::SourcePath> &TranslationOptionCollectionConfusionNet::GetPhrases(size_t startPos, size_t endPos) const
{
  size_t offset = endPos - startPos;
  CHECK(offset < m_collection[startPos].size());
  return m_collection[startPos][offset];
}

std::vector<TranslationOptionCollectionConfusionNet::SourcePath> &TranslationOptionCollectionConfusionNet::GetPhrases(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  CHECK(offset < m_collection[startPos].size());
  return m_collection[startPos][offset];
}

void TranslationOptionCollectionConfusionNet::CreateTranslationOptionsForRange(
  const DecodeGraph &decodeGraph
  , size_t startPos
  , size_t endPos
  , bool adhereTableLimit
  , size_t graphInd)
{
  if ((StaticData::Instance().GetXmlInputType() != XmlExclusive) || !HasXmlOptionsOverlappingRange(startPos,endPos)) {
    Phrase *sourcePhrase = NULL; // can't initialise with substring, in case it's confusion network

    // consult persistent (cross-sentence) cache for stored translation options
    bool skipTransOptCreation = false
                                , useCache = StaticData::Instance().GetUseTransOptCache();
    if (useCache) {
      const WordsRange wordsRange(startPos, endPos);
      sourcePhrase = new Phrase(m_source.GetSubString(wordsRange));

      const TranslationOptionList *transOptList = StaticData::Instance().FindTransOptListInCache(decodeGraph, *sourcePhrase);
      // is phrase in cache?
      if (transOptList != NULL) {
        skipTransOptCreation = true;
        TranslationOptionList::const_iterator iterTransOpt;
        for (iterTransOpt = transOptList->begin() ; iterTransOpt != transOptList->end() ; ++iterTransOpt) {
          TranslationOption *transOpt = new TranslationOption(**iterTransOpt, wordsRange);
          Add(transOpt);
        }
      }
    } // useCache

    if (!skipTransOptCreation) {
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
      int indexStep = 1;

      for (++iterStep; iterStep != decodeGraph.end() ; ++iterStep, ++indexStep) {
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

      } // for (++iterStep

      // add to fully formed translation option list
      PartialTranslOptColl &lastPartialTranslOptColl	= *oldPtoc;
      const vector<TranslationOption*>& partTransOptList = lastPartialTranslOptColl.GetList();
      vector<TranslationOption*>::const_iterator iterColl;
      for (iterColl = partTransOptList.begin() ; iterColl != partTransOptList.end() ; ++iterColl) {
        TranslationOption *transOpt = *iterColl;
        Add(transOpt);
      }

      // storing translation options in persistent cache (kept across sentences)
      if (useCache) {
        if (partTransOptList.size() > 0) {
          TranslationOptionList &transOptList = GetTranslationOptionList(startPos, endPos);
          StaticData::Instance().AddTransOptListToCache(decodeGraph, *sourcePhrase, transOptList);
        }
      }

      lastPartialTranslOptColl.DetachAll();
      totalEarlyPruned += oldPtoc->GetPrunedCount();
      delete oldPtoc;
      // TRACE_ERR( "Early translation options pruned: " << totalEarlyPruned << endl);
    } // if (!skipTransOptCreation)

    if (useCache)
      delete sourcePhrase;
  } // if ((StaticData::Instance().GetXmlInputType() != XmlExclusive) || !HasXmlOptionsOverlappingRange(startPos,endPos))

  if (graphInd == 0 && StaticData::Instance().GetXmlInputType() != XmlPassThrough && HasXmlOptionsOverlappingRange(startPos,endPos)) {
    CreateXmlOptionsForRange(startPos, endPos);
  }
}

} // namespace


