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
  for (size_t phaseSize = 1; phaseSize <= size; ++phaseSize) {
    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
      size_t endPos = startPos + phaseSize -1;
      vector<InputPath*> &vec = m_targetPhrasesfromPt[startPos];

      Phrase subphrase(input.GetSubString(WordsRange(startPos, endPos)));
      WordsRange range(startPos, endPos);

      InputPath *node;
      if (range.GetNumWordsCovered() == 1) {
        node = new InputPath(subphrase, range, NULL, NULL);
        vec.push_back(node);
      } else {
        const InputPath &prevNode = GetInputPath(startPos, endPos - 1);
        node = new InputPath(subphrase, range, &prevNode, NULL);
        vec.push_back(node);
      }

      m_phraseDictionaryQueue.push_back(node);
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

InputPath &TranslationOptionCollectionText::GetInputPath(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  CHECK(offset < m_targetPhrasesfromPt[startPos].size());
  return *m_targetPhrasesfromPt[startPos][offset];
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
  InputPath &inputPath = GetInputPath(startPos, endPos);

  CreateTranslationOptionsForRange(decodeGraph
		  , startPos
		  , endPos
		  , adhereTableLimit
		  , graphInd
		  , inputPath);
}


}



