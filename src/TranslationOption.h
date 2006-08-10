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

#pragma once

#include <vector>
#include "WordsBitmap.h"
#include "WordsRange.h"
#include "Phrase.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "Util.h"
#include "TypeDef.h"
#include "ScoreComponentCollection.h"
#include "StaticData.h"

class PhraseDictionaryBase;
class GenerationDictionary;

/** Available phrase translation for a particular sentence pair.
 * In a multi-factor model, this is expanded from the entries in the 
 * translation tables and generation tables (and pruned to the maximum
 * number allowed). By pre-computing the allowable phrase translations,
 * efficient beam search in Manager is possible when expanding instances
 * of the class Hypothesis - the states in the search.
 * 
 * A translation option contains source and target phrase, aggregate
 * and details scores (in m_scoreBreakdown), including an estimate 
 * how expensive this option will be in search (used to build the 
 * future cost matrix.)
 *
 * m_targetPhrase points to a phrase-table entry.
 * The source word range is zero-indexed, so it can't refer to an empty range. The target phrase may be empty.
 */
class TranslationOption
{
	friend std::ostream& operator<<(std::ostream& out, const TranslationOption& possibleTranslation);

protected:

	Phrase 							m_targetPhrase; /*< output phrase when using this translation option */
	Phrase const*       m_sourcePhrase; /*< input phrase translated by this */
	const WordsRange		m_sourceWordsRange; /*< word position in the input that are covered by this translation option */
	float								m_totalScore; /*< weighted translation costs of this translation option */
	float               m_futureScore; /*< estimate of total cost when using this translation option, includes language model probabilities */
	float               m_partialScore; /*< estimate of the partial cost of a preliminary translation option */

	//! in TranslationOption, m_scoreBreakdown is not complete.  It cannot,
	//! for example, know the full n-gram score since the length of the
	//! TargetPhrase may be shorter than the n-gram order.  But, if it is
	//! possible to estimate, it is included here.
	ScoreComponentCollection2	m_scoreBreakdown;

public:
	TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase);
	TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase, int);

	/** used by initial translation step */
	TranslationOption(const TranslationOption &copy, const TargetPhrase &targetPhrase);

	void MergeFeaturesFromNewPhrase(const TargetPhrase& phrase);
	void MergeFeaturesFromNewPhrase(const Phrase& phrase, const ScoreComponentCollection2& additionalScore);
	
	/** returns target phrase */
	inline const Phrase &GetTargetPhrase() const
	{
		return m_targetPhrase;
	}

	/** returns source word range */
	inline const WordsRange &GetSourceWordsRange() const
	{
		return m_sourceWordsRange;
	}

	/** returns source phrase */
	Phrase const* GetSourcePhrase() const 
	{
	  return m_sourcePhrase;
	}

	bool Overlap(const Hypothesis &hypothesis) const;

	/** return start index of source phrase */
	inline size_t GetStartPos() const
	{
		return m_sourceWordsRange.GetStartPos();
	}

	/** return end index of source phrase */
	inline size_t GetEndPos() const
	{
		return m_sourceWordsRange.GetEndPos();
	}

	/** return length of source phrase */
	inline size_t GetSize() const
	{
		return m_sourceWordsRange.GetEndPos() - m_sourceWordsRange.GetStartPos() + 1;
	}
	/** return source words range */
	inline const WordsRange &GetWordsRange() const
	{
		return m_sourceWordsRange;
	}

	/** return estimate of total cost of this option */
	inline float GetFutureScore() const 	 
	{ 	 
		return m_futureScore; 	 
	}

  /** return true if the source phrase translates into nothing */
	inline bool IsDeletionOption() const
  {
    return m_targetPhrase.GetSize() == 0;
  }

	/** returns detailed component scores */
	inline const ScoreComponentCollection2 &GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}

	void CalcScore();

	TO_STRING;
};


