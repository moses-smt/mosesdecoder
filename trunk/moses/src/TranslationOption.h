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
#include "ScoreComponent.h"

class TranslationOption
{
		friend std::ostream& operator<<(std::ostream& out, const TranslationOption& possibleTranslation);

protected:
	const Phrase 	&m_targetPhrase;
	WordsRange		m_wordsRange;
	float					m_transScore, m_futureScore, m_ngramScore;
#ifdef N_BEST
	ScoreComponent	m_transScoreComponent;
	std::list< std::pair<size_t, float> >	m_lmScoreComponent;
	std::list< std::pair<size_t, float> >	m_trigramComponent;
#endif

	void CalcFutureScore(const LMList &lmList, float weightWP);
public:
	TranslationOption(const WordsRange &wordsRange
										, const TargetPhrase &targetPhrase
										, float transScore
										, float weightWP);
	TranslationOption(const WordsRange &wordsRange
										, const TargetPhrase &targetPhrase
										, float transScore
										, const LMList &lmList
										, float weightWP);

	bool Overlap(const Hypothesis &hypothesis) const;
	inline size_t GetStartPos() const
	{
		return m_wordsRange.GetStartPos();
	}
	inline size_t GetEndPos() const
	{
		return m_wordsRange.GetEndPos();
	}
	inline size_t GetSize() const
	{
		return m_wordsRange.GetEndPos() - m_wordsRange.GetStartPos() + 1;
	}
	inline const WordsRange &GetWordsRange() const
	{
		return m_wordsRange;
	}
	inline const Phrase 	&GetPhrase() const
	{
		return m_targetPhrase;
	}
	inline float GetTranslationScore() const
	{
		return m_transScore;
	}
	inline float GetFutureScore() const
	{
		return m_futureScore;
	}
	inline float GetNgramScore() const
	{
		return m_ngramScore;
	}

#ifdef N_BEST
	inline const ScoreComponent &GetScoreComponents() const
	{
		return m_transScoreComponent;
	}
	inline const std::list< std::pair<size_t, float> > &GetLMScoreComponent() const
	{
		return m_lmScoreComponent;
	}
	inline const std::list< std::pair<size_t, float> > &GetTrigramComponent() const
	{
		return m_trigramComponent;
	}
#endif


};

