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

#include "TranslationOptionCollectionText.h"
#include "Sentence.h"
#include "DecodeStep.h"
#include "DecodeStepTranslation.h"
#include "FactorCollection.h"
#include "WordsRange.h"
#include <list>

using namespace std;

namespace Moses
{
/** constructor; just initialize the base class */
TranslationOptionCollectionText::TranslationOptionCollectionText(Sentence const &input, size_t maxNoTransOptPerCoverage, float translationOptionThreshold)
  : TranslationOptionCollection(input, maxNoTransOptPerCoverage, translationOptionThreshold)
{
  size_t size = input.GetSize();
  m_targetPhrasesfromPt.resize(size);
  for (size_t startPos = 0; startPos < size; ++startPos) {
    std::vector<InputLatticeNode> &vec = m_targetPhrasesfromPt[startPos];
    for (size_t endPos = startPos; endPos < size; ++endPos) {
      Phrase subphrase(input.GetSubString(WordsRange(startPos, endPos)));
      WordsRange range(startPos, endPos);
      InputLatticeNode node(subphrase, range);

      vec.push_back(node);
    }
  }

  for (size_t phaseSize = 1; phaseSize <= size; ++phaseSize) {
    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
      size_t endPos = startPos + phaseSize -1;
      //cerr << startPos << "-" << endPos << "=" << GetPhrase(startPos, endPos) << endl;
      InputLatticeNode &node = GetInputLatticeNode(startPos, endPos);
      m_phraseDictionaryQueue.push_back(&node);
    }
  }
}

/* forcibly create translation option for a particular source word.
	* For text, this function is easy, just call the base class' ProcessOneUnknownWord()
*/
void TranslationOptionCollectionText::ProcessUnknownWord(size_t sourcePos)
{
  const Word &sourceWord = m_source.GetWord(sourcePos);
  ProcessOneUnknownWord(sourceWord,sourcePos);
}

/**
 * Check the source sentence for coverage data
 */
bool TranslationOptionCollectionText::HasXmlOptionsOverlappingRange(size_t startPosition, size_t endPosition) const
{
  Sentence const& source=dynamic_cast<Sentence const&>(m_source);
  return source.XmlOverlap(startPosition,endPosition);
}

/**
 * Create xml-based translation options for the specific input span
 */
void TranslationOptionCollectionText::CreateXmlOptionsForRange(size_t startPosition, size_t endPosition)
{
  Sentence const& source=dynamic_cast<Sentence const&>(m_source);

  vector <TranslationOption*> xmlOptions;

  source.GetXmlTranslationOptions(xmlOptions,startPosition,endPosition);

  //get vector of TranslationOptions from Sentence
  for(size_t i=0; i<xmlOptions.size(); i++) {
    Add(xmlOptions[i]);
  }

};

InputLatticeNode &TranslationOptionCollectionText::GetInputLatticeNode(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  CHECK(offset < m_targetPhrasesfromPt[startPos].size());
  return m_targetPhrasesfromPt[startPos][offset];
}

void TranslationOptionCollectionText::CreateTranslationOptions()
{
  SetTargetPhraseFromPtMatrix();
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
void TranslationOptionCollectionText::CreateTranslationOptionsForRange(
  const DecodeGraph &decodeGraph
  , size_t startPos
  , size_t endPos
  , bool adhereTableLimit
  , size_t graphInd)
{
  InputLatticeNode &inputLatticeNode = GetInputLatticeNode(startPos, endPos);

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

      const PhraseDictionary &phraseDictionary = *decodeStep.GetPhraseDictionaryFeature();
      const TargetPhraseCollection *targetPhrases = inputLatticeNode.GetTargetPhrases(phraseDictionary);

      static_cast<const DecodeStepTranslation&>(decodeStep).ProcessInitialTranslation
      (m_source, *oldPtoc
       , startPos, endPos, adhereTableLimit
       , targetPhrases);

      // do rest of decode steps
      int indexStep = 0;

      for (++iterStep ; iterStep != decodeGraph.end() ; ++iterStep) {

        const DecodeStep *decodeStep = *iterStep;
        PartialTranslOptColl* newPtoc = new PartialTranslOptColl;

        // go thru each intermediate trans opt just created
        const vector<TranslationOption*>& partTransOptList = oldPtoc->GetList();
        vector<TranslationOption*>::const_iterator iterPartialTranslOpt;
        for (iterPartialTranslOpt = partTransOptList.begin() ; iterPartialTranslOpt != partTransOptList.end() ; ++iterPartialTranslOpt) {
          TranslationOption &inputPartialTranslOpt = **iterPartialTranslOpt;

          if (const DecodeStepTranslation *translateStep = dynamic_cast<const DecodeStepTranslation*>(decodeStep) ) {
            const PhraseDictionary &phraseDictionary = *translateStep->GetPhraseDictionaryFeature();
            const TargetPhraseCollection *targetPhrases = inputLatticeNode.GetTargetPhrases(phraseDictionary);
            translateStep->Process(inputPartialTranslOpt
                             , *decodeStep
                             , *newPtoc
                             , this
                             , adhereTableLimit
                             , *sourcePhrase
                             , targetPhrases);
          }
          else {
            decodeStep->Process(inputPartialTranslOpt
                             , *decodeStep
                             , *newPtoc
                             , this
                             , adhereTableLimit
                             , *sourcePhrase);
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


}



