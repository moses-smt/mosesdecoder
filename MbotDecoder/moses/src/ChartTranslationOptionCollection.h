// $Id: ChartTranslationOptionCollection.h,v 1.1.1.1 2013/01/06 16:54:16 braunefe Exp $
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

namespace Moses
{
class DecodeGraph;
class Word;
class ChartTranslationOption;
class DottedRule;
class WordPenaltyProducer;
class ChartCellCollection;

class ChartTranslationOptionCollection
{
  friend std::ostream& operator<<(std::ostream&, const ChartTranslationOptionCollection&);
protected:
  const InputType &m_source;
  const TranslationSystem* m_system;
  std::vector <DecodeGraph*> m_decodeGraphList;
  const ChartCellCollection &m_hypoStackColl;
  const std::vector<ChartRuleLookupManager*> &m_ruleLookupManagers;

  std::vector< std::vector< ChartTranslationOptionList > > m_collection; /*< contains translation options */
  std::vector<Phrase*> m_unksrcs;
  std::list<TargetPhraseCollection*> m_cacheTargetPhraseCollection;
  std::list<std::vector<DottedRule*>* > m_dottedRuleCache;

  //new : need a cache for mbot dotted rules
  std::list<std::vector<DottedRuleMBOT*>* > m_mbotDottedRuleCache;

  // for adding 1 trans opt in unknown word proc
  void Add(ChartTranslationOption *transOpt, size_t pos);

  ChartTranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos);
  const ChartTranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos) const;

  void ProcessUnknownWord(size_t startPos, size_t endPos);

  // taken from ChartTranslationOptionCollectionText.
  void ProcessUnknownWord(size_t sourcePos);

  //! special handling of ONE unknown words.
  virtual void ProcessOneUnknownWord(const Word &sourceWord
                                     , size_t sourcePos, size_t length = 1);

  void ProcessUnknownWordMBOT(size_t startPos, size_t endPos);

  // taken from ChartTranslationOptionCollectionText.
  void ProcessUnknownWordMBOTWithSourceLabel(size_t sourcePos,size_t endPos);

  //! special handling of ONE unknown words.
  virtual void ProcessOneUnknownWordMBOT(const Word &sourceWord
                                     , size_t sourcePos, std::vector<Word> sourceLabels, size_t length = 1);

  //! pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
  void Prune(size_t startPos, size_t endPos);

  //! sort all trans opt in each list for cube pruning */
  void Sort(size_t startPos, size_t endPos);

   //! sort all trans opt in each list for cube pruning */
  void SortMBOT(size_t startPos, size_t endPos);

public:
  ChartTranslationOptionCollection(InputType const& source
                              , const TranslationSystem* system
                              , const ChartCellCollection &hypoStackColl
                              , const std::vector<ChartRuleLookupManager*> &ruleLookupManagers);
  virtual ~ChartTranslationOptionCollection();
  void CreateTranslationOptionsForRange(size_t startPos
                                        , size_t endPos);

  const ChartTranslationOptionList &GetTranslationOptionList(const WordsRange &range) const {
    return GetTranslationOptionList(range.GetStartPos(), range.GetEndPos());
  }

};

}

