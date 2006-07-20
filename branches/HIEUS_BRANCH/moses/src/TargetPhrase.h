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
#include "Phrase.h"
#include "ScoreComponent.h"
#include "ScoreColl.h"

class LMList;
class PhraseDictionary;
class GenerationDictionary;

class TargetPhrase: public Phrase
{
  friend std::ostream& operator<<(std::ostream&, const TargetPhrase&);
protected:
	float m_transScore, m_ngramScore, m_fullScore;
#ifdef N_BEST
	ScoreComponent m_scoreComponent;
	std::list< std::pair<size_t, float> > m_lmScoreComponent;
	ScoreColl m_ngramComponent;
#endif

public:

	TargetPhrase(FactorDirection direction, const PhraseDictionary *phraseDictionary);
	TargetPhrase(FactorDirection direction);
		// unknown word
	TargetPhrase(const TargetPhrase& phrase)
	: Phrase(phrase)
	, m_transScore(phrase.m_transScore)
	, m_ngramScore(phrase.m_ngramScore)
	, m_fullScore(phrase.m_fullScore)
#ifdef N_BEST
	, m_scoreComponent(phrase.m_scoreComponent)
	, m_lmScoreComponent(phrase.m_lmScoreComponent)
	, m_ngramComponent(phrase.m_ngramComponent)
#endif
	{ // deep copy
	}

	void SetScore(const std::vector<float> &scoreVector, const std::vector<float> &weightT,
								const LMList &languageModels, float weightWP);
	void SetScore(float weightWP);
	// used when creating translations of unknown words:
	void ResetScore();
	void SetWeights(const std::vector<float> &weightT);
	TargetPhrase *MergeNext(const TargetPhrase &targetPhrase) const;
		// used for translation step
	
  inline float GetTranslationScore() const
  {
    return m_transScore;
  }
  //TODO is this really the best name?
  inline float GetFutureScore() const
  {
    return m_fullScore;
  }
  inline float GetNgramScore() const
  {
    return m_ngramScore;
  }

#ifdef N_BEST
	inline const ScoreComponent &GetScoreComponents() const
	{
		return m_scoreComponent;
	}
  inline const std::list< std::pair<size_t, float> > &GetLMScoreComponent() const
  {
    return m_lmScoreComponent;
  }
  inline const ScoreColl &GetNgramComponent() const
  {
    return m_ngramComponent;
  }
#endif

};

std::ostream& operator<<(std::ostream&, const TargetPhrase&);

