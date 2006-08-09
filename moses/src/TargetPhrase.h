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
#include "ScoreComponentCollection.h"

class LMList;
class PhraseDictionaryBase;
class GenerationDictionary;
class ScoreProducer;

/** TargetPhrase represents a full entry in a phrase table (scores, translation) EXCEPT for the
 * French side of the rule.
 */
class TargetPhrase: public Phrase
{
	friend std::ostream& operator<<(std::ostream&, const TargetPhrase&);
protected:
	float m_transScore, m_ngramScore, m_fullScore;
	ScoreComponentCollection2 m_scoreBreakdown;

	// in case of confusion net, ptr to source phrase
	Phrase const* m_sourcePhrase; 
public:
	TargetPhrase(FactorDirection direction=Output);

	//! used by the unknown word handler- these targets
	//! don't have a translation score, so wp is the only thing used
	void SetScore();

	/*** Called immediately after creation to initialize scores.
   *
   * @param translationScoreProducer The PhraseDictionary that this TargetPhrase is contained by.
   *        Used to identify where the scores for this phrase belong in the list of all scores.
   * @param scoreVector the vector of scores (log probs) associated with this translation
   * @param weighT the weights for the individual scores (t-weights in the .ini file)
   * @param languageModels all the LanguageModels that should be used to compute the LM scores
   * @param weightWP the weight of the word penalty
   *
   * @TODO should this be part of the constructor?  If not, add explanation why not.
   */
	void SetScore(const ScoreProducer* translationScoreProducer,
								const std::vector<float> &scoreVector,
								const std::vector<float> &weightT,
								float weightWP,
								const LMList &languageModels);

	// used when creating translations of unknown words:
	void ResetScore();
	void SetWeights(const ScoreProducer*, const std::vector<float> &weightT);

	TargetPhrase *MergeNext(const TargetPhrase &targetPhrase) const;
		// used for translation step
	
  inline float GetTranslationScore() const
  {
    return m_transScore;
  }
  /***
   * return the estimated score resulting from our being added to a sentence
   * (it's an estimate because we don't have full n-gram info for the language model
   *  without using the (unknown) full sentence)
   * 
   */
  inline float GetFutureScore() const
  {
    return m_fullScore;
  }
  inline float GetNgramScore() const
  {
    return m_ngramScore;
  }
	inline const ScoreComponentCollection2 &GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}

	void SetSourcePhrase(Phrase const* p) 
	{
		m_sourcePhrase=p;
	}
	Phrase const* GetSourcePhrase() const 
	{
		return m_sourcePhrase;
	}

	TO_STRING;
};

std::ostream& operator<<(std::ostream&, const TargetPhrase&);

