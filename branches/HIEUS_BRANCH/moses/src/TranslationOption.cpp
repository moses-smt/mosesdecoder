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

TranslationOption::TranslationOption(const WordsRange &wordsRange, const TargetPhrase &targetPhrase)
: m_phrase(targetPhrase)
,m_sourceWordsRange	(wordsRange)
{	// used by initial translation step

	// set score
	m_scoreGen		= 0;
	m_scoreTrans	= targetPhrase.GetTranslationScore();
#ifdef N_BEST
	m_transScoreComponent.Add(targetPhrase.GetScoreComponents());
#endif
}

TranslationOption::TranslationOption(const TranslationOption &copy, const TargetPhrase &targetPhrase)
: m_phrase(targetPhrase)
,m_sourceWordsRange	(copy.m_sourceWordsRange)
#ifdef N_BEST
,m_transScoreComponent(copy.m_transScoreComponent)
,m_generationScoreComponent(copy.m_generationScoreComponent)
#endif
{ // used in creating the next translation step

	m_scoreTrans	+= targetPhrase.GetTranslationScore();
	
#ifdef N_BEST
	m_transScoreComponent.Add(targetPhrase.GetScoreComponents());
#endif
}

TranslationOption *TranslationOption::MergeTranslation(const TargetPhrase &targetPhrase) const
{
	TranslationOption *newTransOpt = new TranslationOption(*this, targetPhrase);
	return newTransOpt;
}

bool TranslationOption::Overlap(const Hypothesis &hypothesis) const
{
	const WordsBitmap &bitmap = hypothesis.GetWordsBitmap();
	return bitmap.Overlap(GetSourceWordsRange());
}

// friend
ostream& operator<<(ostream& out, const TranslationOption& possibleTranslation)
{
	out << possibleTranslation.GetTargetPhrase() 
			<< ", pC=" << possibleTranslation.GetTranslationScore();
	return out;
}

