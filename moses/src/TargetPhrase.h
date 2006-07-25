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
class PhraseDictionaryBase;
class GenerationDictionary;

class TargetPhrase: public Phrase
{
	friend std::ostream& operator<<(std::ostream&, const TargetPhrase&);
protected:
	float m_transScore, m_ngramScore, m_fullScore;
#ifdef N_BEST
	float m_inputScore;
	ScoreComponent m_scoreComponent;
	std::vector< std::pair<size_t, float> > m_lmScoreComponent;
	ScoreColl m_ngramComponent;
#endif

public:

	TargetPhrase(FactorDirection direction, const PhraseDictionaryBase *phraseDictionary);
	TargetPhrase(FactorDirection direction);
		// unknown word

	void SetScore(float weightWP);
	void SetScore(const std::vector<float> &scoreVector, const std::vector<float> &weightT,
								const LMList &languageModels, float weightWP, float inputScore=0.0, float weightInput=0.0);
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
  /***
   * return the estimated score resulting from our being added to a sentence
   * (it's an estimate because we don't have full n-gram info for the language model
   *  without using the (unknown) full sentence)
   * 
   * TODO is this really the best name?
   */

#ifdef N_BEST
	inline const ScoreComponent &GetScoreComponents() const
	{
		return m_scoreComponent;
	}
  inline const ScoreColl &GetNgramComponent() const
  {
    return m_ngramComponent;
  }
#endif

};

std::ostream& operator<<(std::ostream&, const TargetPhrase&);

