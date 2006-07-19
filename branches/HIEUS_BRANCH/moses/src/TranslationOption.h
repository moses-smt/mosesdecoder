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

#include "WordsBitmap.h"
#include "TargetPhrase.h"
#include "Hypothesis.h"
#include "Util.h"
#include "TypeDef.h"
#include "ScoreComponentCollection.h"

class GenerationDictionary;

/***
 * Specify source and target words for a possible translation. m_targetPhrase points to a phrase-table entry.
 * The source word range is zero-indexed, so it can't refer to an empty range.
 */
class TranslationOption
{
		friend std::ostream& operator<<(std::ostream& out, const TranslationOption& possibleTranslation);

protected:

	const Phrase 				&m_phrase;
	const WordsRange		m_sourceWordsRange;
	float								m_scoreTrans, m_scoreGen, m_futureScore, m_ngramScore;
#ifdef N_BEST
	ScoreComponentCollection	m_transScoreComponent;
	ScoreColl									m_generationScoreComponent;
#endif

public:
	TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase);
	// used by initial translation step
	TranslationOption(const TranslationOption &copy, const TargetPhrase &targetPhrase);
	// used by MergeTranslation 
	TranslationOption(const TranslationOption &copy
											, const Phrase &inputPhrase
											, const GenerationDictionary *generationDictionary
											, float generationScore
											, float weight);

	TranslationOption *MergeTranslation(const TargetPhrase &targetPhrase) const;
	TranslationOption *MergeGeneration(const Phrase &inputPhrase
																		, const GenerationDictionary *generationDictionary
																		, float generationScore
																		, float weight) const;

	inline const Phrase &GetTargetPhrase() const
	{
		return m_phrase;
	}
	inline const WordsRange &GetSourceWordsRange() const
	{
		return m_sourceWordsRange;
	}

	bool Overlap(const Hypothesis &hypothesis) const;
	inline size_t GetStartPos() const
	{
		return m_sourceWordsRange.GetStartPos();
	}
	inline size_t GetEndPos() const
	{
		return m_sourceWordsRange.GetEndPos();
	}
	inline size_t GetSize() const
	{
		return m_sourceWordsRange.GetEndPos() - m_sourceWordsRange.GetStartPos() + 1;
	}
	inline float GetTranslationScore() const
	{
		return m_scoreTrans;
	}
	inline float GetGenerationScore() const
	{
		return m_scoreGen;
	}
	inline float GetFutureScore() const 	 
	{ 	 
				 return m_futureScore; 	 
	}
	inline float GetNgramScore() const 	 
  { 	 
		return m_ngramScore; 	 
	}
	void CalcScore(const LMList &allLM, float weightWordPenalty);

#ifdef N_BEST
	inline const ScoreComponentCollection &GetScoreComponentCollection() const
	{
		return m_transScoreComponent;
	}
#endif


};
