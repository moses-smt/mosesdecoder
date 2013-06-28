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
#include "FactorCollection.h"
#include "WordsRange.h"

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


}



