// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang

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

#include "WordsRange.h"
#include "StackVec.h"

#include <list>
#include <vector>

namespace Moses
{

class ChartParserCallback;
class ChartRuleLookupManager;
class InputType;
class TranslationSystem;
class ChartCellCollectionBase;
class Word;
class Phrase;
class TargetPhraseCollection;
class DecodeGraph;

class ChartParserUnknown {
  public:
    ChartParserUnknown(const TranslationSystem &system);
    ~ChartParserUnknown();

    void Process(const Word &sourceWord, const WordsRange &range, ChartParserCallback &to);

  private:
    const TranslationSystem &m_system;
    std::vector<Phrase*> m_unksrcs;
    std::list<TargetPhraseCollection*> m_cacheTargetPhraseCollection;
    StackVec m_emptyStackVec;
};

class ChartParser {
  public:
    ChartParser(const InputType &source, const TranslationSystem &system, ChartCellCollectionBase &cells);
    ~ChartParser();

    void Create(const WordsRange &range, ChartParserCallback &to);

  private:
    ChartParserUnknown m_unknown;
    std::vector <DecodeGraph*> m_decodeGraphList;
    std::vector<ChartRuleLookupManager*> m_ruleLookupManagers;
    InputType const& m_source; /**< source sentence to be translated */
};

}

