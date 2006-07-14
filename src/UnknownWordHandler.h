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

#pragma once

#include <list>
#include "TargetPhrase.h"
#include "WordsRange.h"
#include "PhraseDictionary.h"
#include "TranslationOption.h"

class StaticData;

/***
 * Provide analysis of source-language words the phrase table can't help us with. This default implementation
 * assumes all unknown words are proper names; it's meant to be inherited. The unknown-word handler used 
 * is set in main().
 */
class UnknownWordHandler
{
	public:
	
		virtual ~UnknownWordHandler() {}
	
		/***
		 * \param sourceWordsRange A group of consecutive source words we can't translate via the phrase table
		 * \param sourcePhrase The source words to be translated
		 * \param staticData
		 * \param phraseDictionary A modifiable phrase table
		 * \return A list of possible translations for the given source phrase
		 */
		virtual std::list<TranslationOption> GetPossibleTranslations(
			const WordsRange& sourceWordsRange, const Phrase& sourcePhrase, StaticData& staticData, PhraseDictionary& phraseDictionary);
};
