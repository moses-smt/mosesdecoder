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

#include <vector>
#include "../../moses/src/InputType.h"
#include "../../moses/src/DecodeGraph.h"
#include "../../moses/src/ChartTranslationOptionList.h"
#include "../../moses/src/ChartRuleLookupManager.h"

namespace Moses
{
	class DecodeGraph;
	class Word;
	class ChartTranslationOption;
	class WordConsumed;
  class WordPenaltyProducer;
};

namespace MosesChart
{
class ChartCellCollection;

class TranslationOptionCollection
{
	friend std::ostream& operator<<(std::ostream&, const TranslationOptionCollection&);
protected:
	const Moses::InputType		&m_source;
	const Moses::TranslationSystem* m_system;
  std::vector <Moses::DecodeGraph*> m_decodeGraphList;
	const ChartCellCollection &m_hypoStackColl;
  const std::vector<Moses::ChartRuleLookupManager*> &m_ruleLookupManagers;

	std::vector< std::vector< Moses::ChartTranslationOptionList > >	m_collection; /*< contains translation options */
	std::vector<Moses::Phrase*> m_unksrcs;
	std::list<Moses::TargetPhrase*> m_cacheTargetPhrase;
	std::list<std::vector<Moses::WordConsumed*>* > m_cachedWordsConsumed;
	
	// for adding 1 trans opt in unknown word proc
	void Add(Moses::ChartTranslationOption *transOpt, size_t pos);

	Moses::ChartTranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos);
	const Moses::ChartTranslationOptionList &GetTranslationOptionList(size_t startPos, size_t endPos) const;

	void ProcessUnknownWord(size_t startPos, size_t endPos);

	// taken from TranslationOptionCollectionText.
	void ProcessUnknownWord(size_t sourcePos);

	//! special handling of ONE unknown words.
	virtual void ProcessOneUnknownWord(const Moses::Word &sourceWord
																		 , size_t sourcePos, size_t length = 1);
	
	//! pruning: only keep the top n (m_maxNoTransOptPerCoverage) elements */
	void Prune(size_t startPos, size_t endPos);

	//! sort all trans opt in each list for cube pruning */
	void Sort(size_t startPos, size_t endPos);

public:
	TranslationOptionCollection(Moses::InputType const& source
														, const Moses::TranslationSystem* system
														, const ChartCellCollection &hypoStackColl
														, const std::vector<Moses::ChartRuleLookupManager*> &ruleLookupManagers);
	virtual ~TranslationOptionCollection();
	void CreateTranslationOptionsForRange(size_t startPos
																			, size_t endPos);

	const Moses::ChartTranslationOptionList &GetTranslationOptionList(const Moses::WordsRange &range) const
	{
		return GetTranslationOptionList(range.GetStartPos(), range.GetEndPos());
	}

};

}

