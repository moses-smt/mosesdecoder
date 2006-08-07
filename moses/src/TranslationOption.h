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

class PhraseDictionaryBase;
class GenerationDictionary;

/***
 * Specify source and target words for a possible translation. m_targetPhrase points to a phrase-table entry.
 * The source word range is zero-indexed, so it can't refer to an empty range. The target phrase may be empty.
 */
class TranslationOption
{
	friend std::ostream& operator<<(std::ostream& out, const TranslationOption& possibleTranslation);

protected:

	const Phrase 				m_targetPhrase; /*< output phrase when using this translation option */
	Phrase const*       m_sourcePhrase; /*< input phrase translated by this */
	const WordsRange		m_sourceWordsRange; /*< word position in the input that are covered by this translation option */
	float								m_totalScore; /*< weighted translation costs of this translation option */
	float               m_futureScore; /*< estimate of total cost when using this translation option, includes language model probabilities */

	//! in TranslationOption, m_scoreBreakdown is not complete.  It cannot,
	//! for example, know the full n-gram score since the length of the
	//! TargetPhrase may be shorter than the n-gram order.  But, if it is
	//! possible to estimate, it will be known
	ScoreComponentCollection2	m_scoreBreakdown;

public:
	TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase);
	/** used by initial translation step */
	TranslationOption(const TranslationOption &copy, const TargetPhrase &targetPhrase);
	/** used by MergeTranslation */
	TranslationOption(const TranslationOption &copy
											, const Phrase &inputPhrase
											, const ScoreComponentCollection2& additionalScore);
	/** used to create trans opt from unknown word */
	TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase, int /*whatever*/);
	
	/** add factors from a translation step */
	TranslationOption *MergeTranslation(const TargetPhrase &targetPhrase) const;
	/** add factors from a generation step */
	TranslationOption *MergeGeneration(const Phrase &inputPhrase
																		, const ScoreComponentCollection2& generationScore) const;

	inline const Phrase &GetTargetPhrase() const
	{
		return m_targetPhrase;
	}
	inline const WordsRange &GetSourceWordsRange() const
	{
		return m_sourceWordsRange;
	}
	Phrase const* GetSourcePhrase() const 
	{
	  return m_sourcePhrase;
	}

	bool Overlap(const Hypothesis &hypothesis) const;
	/***
	 * return start index of source phrase
	 */
	inline size_t GetStartPos() const
	{
		return m_sourceWordsRange.GetStartPos();
	}
	/***
	 * return end index of source phrase
	 */
	inline size_t GetEndPos() const
	{
		return m_sourceWordsRange.GetEndPos();
	}
	/***
	 * return length of source phrase
	 */
	inline size_t GetSize() const
	{
		return m_sourceWordsRange.GetEndPos() - m_sourceWordsRange.GetStartPos() + 1;
	}
	/***
	 * return source words range
	 */
	inline const WordsRange &GetWordsRange() const
	{
		return m_sourceWordsRange;
	}
	inline float GetFutureScore() const 	 
	{ 	 
		return m_futureScore; 	 
	}
  /***
   * returns true if the source phrase translates into nothing
   */
	inline bool IsDeletionOption() const
  {
    return m_targetPhrase.GetSize() == 0;
  }
	void CalcScore(const LMList &allLM, float weightWordPenalty);

	inline const ScoreComponentCollection2 &GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}

	TO_STRING;
};


