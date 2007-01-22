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

#include <string>
#include <vector>
#include "Factor.h"
#include "TypeDef.h"
#include "Util.h"
#include "ScoreProducer.h"
#include "Word.h"

class FactorCollection;
class Factor;
class Phrase;

//! Abstract base class which represent a language model on a contiguous phrase
class LanguageModel : public ScoreProducer
{
protected:	
	float				m_weight; //! scoring weight. Shouldn't this now be superceded by ScoreProducer???
	std::string	m_filePath; //! for debugging purposes
	size_t			m_nGramOrder; //! max n-gram length contained in this LM
	Word m_sentenceStartArray, m_sentenceEndArray; //! Contains factors which represents the beging and end words for this LM. 
																								//! Usually <s> and </s>

	/** constructor to be called by inherited class
	 * \param registerScore whether this LM will be directly used to score sentence. 
	 * 						Usually true, except where LM is a component in a composite LM, eg. LanguageModelJoint
	 */
	LanguageModel(bool registerScore);

public:
	/* Returned from LM implementations which points at the state used. For example, if a trigram score was requested
	 * but the LM backed off to using the trigram, the State pointer will point to the bigram.
	 * Used for more agressive pruning of hypothesis
	 */
  typedef const void* State;

	virtual ~LanguageModel();

	//! see ScoreProducer.h
	size_t GetNumScoreComponents() const;

	//! Single or multi-factor
	virtual LMType GetLMType() const = 0;

	/* whether this LM can be used on a particular phrase. 
	 * Should return false if phrase size = 0 or factor types required don't exists
	 */
	virtual bool Useable(const Phrase &phrase) const = 0;

	/* calc total unweighted LM score of this phrase and return score via arguments.
	 * Return scores should always be in natural log, regardless of representation with LM implementation.
	 * Uses GetValue() of inherited class.
	 * Useable() should be called beforehand on the phrase
	 * \param fullScore scores of all unigram, bigram... of contiguous n-gram of the phrase
	 * \param ngramScore score of only n-gram of order m_nGramOrder
	 */
	void CalcScore(const Phrase &phrase
							, float &fullScore
							, float &ngramScore) const;
	/* get score of n-gram. n-gram should not be bigger than m_nGramOrder
	 * Specific implementation can return State and len data to be used in hypothesis pruning
	 * \param contextFactor n-gram to be scored
	 * \param finalState state used by LM. Return arg
	 * \param len ???
	 */
	virtual float GetValue(const std::vector<const Word*> &contextFactor
												, State* finalState = 0
												, unsigned int* len = 0) const = 0;
	//! get State for a particular n-gram
	State GetState(const std::vector<const Word*> &contextFactor, unsigned int* len = 0) const;

	//! max n-gram order of LM
	size_t GetNGramOrder() const
	{
		return m_nGramOrder;
	}
	
	//! Contains factors which represents the beging and end words for this LM. Usually <s> and </s>
	const Word &GetSentenceStartArray() const
	{
		return m_sentenceStartArray;
	}
	const Word &GetSentenceEndArray() const
	{
		return m_sentenceEndArray;
	}
	
	//! scoring weight. Shouldn't this now be superceded by ScoreProducer???
	float GetWeight() const
	{
		return m_weight;
	}
	void SetWeight(float weight)
	{
		m_weight = weight;
	}
	
	virtual const std::string GetScoreProducerDescription(int idx = 0) const = 0;
  
  //! overrideable funtions for IRST LM to cleanup. Maybe something to do with on demand/cache loading/unloading
  virtual void InitializeBeforeSentenceProcessing(){};
  virtual void CleanUpAfterSentenceProcessing() {};  
};

