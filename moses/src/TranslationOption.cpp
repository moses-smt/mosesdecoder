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

#include "TranslationOption.h"
#include "WordsBitmap.h"

using namespace std;

TranslationOption::TranslationOption(const WordsRange &wordsRange
										, const TargetPhrase &targetPhrase
										, float transScore
										, float weightWP)
: m_targetPhrase(targetPhrase)
,m_wordsRange	(wordsRange)
,m_transScore	(transScore)
,m_futureScore	(0)
,m_ngramScore(0)
#ifdef N_BEST
,m_ScoreComponent(targetPhrase.GetScoreComponents())
#endif
{
}

TranslationOption::TranslationOption(const WordsRange &wordsRange
																			, const TargetPhrase &targetPhrase
																			, float transScore
																			, const LMList &lmList
																			, float weightWP)
: m_targetPhrase(targetPhrase)
,m_wordsRange	(wordsRange)
,m_transScore	(transScore)
#ifdef N_BEST
,m_ScoreComponent(targetPhrase.GetScoreComponents())
#endif
{
	CalcFutureScore(lmList, weightWP);
}

bool TranslationOption::Overlap(const Hypothesis &hypothesis) const
{
	const WordsBitmap &bitmap = hypothesis.GetWordsBitmap();
	return bitmap.Overlap(GetWordsRange());
}

void TranslationOption::CalcFutureScore(const LMList &lmList, float weightWP)
{
	m_futureScore = 0;
	m_ngramScore= 0;

	LMList::const_iterator iterLM;
	for (iterLM = lmList.begin() ; iterLM != lmList.end() ; ++iterLM)
	{
		const LanguageModel &languageModel = **iterLM;
		const float weightLM = languageModel.GetWeight();
		float fullScore, nGramScore;
#ifdef N_BEST
		languageModel.CalcScore(m_targetPhrase, fullScore, nGramScore, m_trigramComponent);
#endif
#ifndef N_BEST
		languageModel.CalcScore(m_targetPhrase, fullScore, nGramScore, *static_cast< list< pair<size_t, float> >* > (NULL));
#endif

		// total LM score so far
		m_futureScore += fullScore * weightLM;
		m_ngramScore	+= nGramScore * weightLM;
		
	}

	size_t phraseSize = m_targetPhrase.GetSize();
	m_futureScore += m_transScore - phraseSize * weightWP;	
}

// friend
ostream& operator<<(ostream& out, const TranslationOption& possibleTranslation)
{
	out << "(" 
			<< possibleTranslation.GetPhrase() 
			<< ") ";
	return out;
}

