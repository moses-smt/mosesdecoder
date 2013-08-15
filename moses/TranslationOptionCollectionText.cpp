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
  m_inputPathMatrix.resize(size);
  for (size_t phaseSize = 1; phaseSize <= size; ++phaseSize) {
    for (size_t startPos = 0; startPos < size - phaseSize + 1; ++startPos) {
      size_t endPos = startPos + phaseSize -1;
      vector<InputPath*> &vec = m_inputPathMatrix[startPos];

      WordsRange range(startPos, endPos);
      Phrase subphrase(input.GetSubString(WordsRange(startPos, endPos)));
      const NonTerminalSet &labels = input.GetLabelSet(startPos, endPos);

      InputPath *node;
      if (range.GetNumWordsCovered() == 1) {
        node = new InputPath(subphrase, labels, range, NULL, NULL);
        vec.push_back(node);
      } else {
        const InputPath &prevNode = GetInputPath(startPos, endPos - 1);
        node = new InputPath(subphrase, labels, range, &prevNode, NULL);
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
  const InputPath &inputPath = GetInputPath(sourcePos, sourcePos);
  ProcessOneUnknownWord(inputPath,sourcePos);
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
 * Check if the given translation option violates a specified xml Option
 */
bool TranslationOptionCollectionText::ViolatesXmlOptionsConstraint(size_t startPosition, size_t endPosition, TranslationOption *transOpt) const
{
  // skip if there is no overlap
  Sentence const& source=dynamic_cast<Sentence const&>(m_source);
  if (!source.XmlOverlap(startPosition,endPosition)) {
    return false;
  }
  // check for all sub spans
  for(size_t start=startPosition; start<=endPosition; start++) {
    for(size_t end=start; end<=endPosition; end++) {
      // get list of xml options for the subspan
      vector <TranslationOption*> xmlOptions;
      source.GetXmlTranslationOptions(xmlOptions,start,end);
      if (xmlOptions.size() > 0) {
        // check if any xml option matches
        for(size_t i=0; i<xmlOptions.size(); i++) {
          const TargetPhrase &phrase = transOpt->GetTargetPhrase();
          const TargetPhrase &xmlPhrase = xmlOptions[i]->GetTargetPhrase();
          // check this xml option (if shorter)
          if (phrase.GetSize() >= xmlPhrase.GetSize()) {
            // match may start in middle of phrase
            for(size_t offset=0; offset <= phrase.GetSize()-xmlPhrase.GetSize(); offset++) {
              bool match = true;
              // match every word (only surface factor)
              for(size_t wordPos=0; match && wordPos < xmlPhrase.GetSize(); wordPos++) {
                if (phrase.GetFactor( wordPos+offset,0 )->Compare(*(xmlPhrase.GetFactor( wordPos,0 )))) {
                  match = false;
                }
              }
              if (match) {
		return false; // no violation if matching xml option found
              }
            }
          }
        }
	return true; // there were xml options for this range, but none matched
      } 
    }
  }
  return false;
}

/**
 * Create xml-based translation options for the specific input span
 */
void TranslationOptionCollectionText::CreateXmlOptionsForRange(size_t startPos, size_t endPos)
{
  Sentence const& source=dynamic_cast<Sentence const&>(m_source);
  InputPath &inputPath = GetInputPath(startPos,endPos);

  vector <TranslationOption*> xmlOptions;
  source.GetXmlTranslationOptions(xmlOptions,startPos,endPos);

  //get vector of TranslationOptions from Sentence
  for(size_t i=0; i<xmlOptions.size(); i++) {
    TranslationOption *transOpt = xmlOptions[i];
    transOpt->SetInputPath(inputPath);
    Add(transOpt);
  }

};

InputPath &TranslationOptionCollectionText::GetInputPath(size_t startPos, size_t endPos)
{
  size_t offset = endPos - startPos;
  CHECK(offset < m_inputPathMatrix[startPos].size());
  return *m_inputPathMatrix[startPos][offset];
}

void TranslationOptionCollectionText::CreateTranslationOptions()
{
  GetTargetPhraseCollectionBatch();
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

  TranslationOptionCollection::CreateTranslationOptionsForRange(decodeGraph
      , startPos
      , endPos
      , adhereTableLimit
      , graphInd
      , inputPath);
}


}

