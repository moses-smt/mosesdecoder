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

#include <vector>
#include "InputType.h"
#include "DecodeGraph.h"
#include "ChartTranslationOptionList.h"
#include "ChartRuleLookupManager.h"
#include "StackVec.h"

namespace Moses
{
class DecodeGraph;
class Word;
class ChartTranslationOption;
class DottedRule;
class WordPenaltyProducer;
class ChartCellCollection;

/** list of list of ALL target phrases for a sentence. NOT translation option
 * 1 of these is held by the chart manager
 *  @todo Why is the manager holding target phrases, and not the translation option, or it's collection?
 *  @todo Why isn't each TargetPhraseCollection held in separate chart cell?
 */
class ChartTranslationOptionCollection
{
protected:
  const InputType &m_source;
  const TranslationSystem* m_system;
  std::vector <DecodeGraph*> m_decodeGraphList;
  const ChartCellCollection &m_hypoStackColl;
  const std::vector<ChartRuleLookupManager*> &m_ruleLookupManagers;

  ChartTranslationOptionList m_translationOptionList;
  std::vector<Phrase*> m_unksrcs;
  std::list<TargetPhraseCollection*> m_cacheTargetPhraseCollection;
  StackVec m_emptyStackVec;

  //! special handling of ONE unknown words.
  virtual void ProcessOneUnknownWord(const Word &, const WordsRange &);

public:
  ChartTranslationOptionCollection(InputType const& source
                              , const TranslationSystem* system
                              , const ChartCellCollection &hypoStackColl
                              , const std::vector<ChartRuleLookupManager*> &ruleLookupManagers);
  virtual ~ChartTranslationOptionCollection();
  void CreateTranslationOptionsForRange(const WordsRange &);

  const ChartTranslationOptionList &GetTranslationOptionList() const {
    return m_translationOptionList;
  }

  void Clear() { m_translationOptionList.Clear(); }

};

}
