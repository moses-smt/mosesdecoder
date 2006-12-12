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
#include "LanguageModel.h"
#include "PhraseDictionaryMemory.h"
#include "FactorCollection.h"
#include "WordsRange.h"
#include "LMList.h"

using namespace std;

/** constructor; just initialize the base class */
TranslationOptionCollectionText::TranslationOptionCollectionText(Sentence const &inputSentence, size_t maxNoTransOptPerCoverage) 
	: TranslationOptionCollection(inputSentence, maxNoTransOptPerCoverage) {}

/* forcibly create translation option for a particular source word.
	* For text, this function is easy, just call the base class' ProcessOneUnknownWord()
*/
void TranslationOptionCollectionText::ProcessUnknownWord(
										size_t decodeStepId
										, size_t sourcePos
										, FactorCollection &factorCollection)
{
	const Word &sourceWord = m_source.GetWord(sourcePos);
	ProcessOneUnknownWord(decodeStepId, sourceWord,sourcePos,factorCollection);
}
