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

#include <iostream>
#include <vector>
#include "Phrase.h"
#include "TypeDef.h"
#include "WordsBitmap.h"
#include "Sentence.h"
#include "Phrase.h"
#include "PhraseDictionary.h"
#include "GenerationDictionary.h"
#include "LanguageModelSingleFactor.h"
#include "ScoreComponentCollection.h"
#include "LexicalReordering.h"
#include "Input.h"
#include "ObjectPool.h"

class SquareMatrix;
class StaticData;
class TranslationOption;
class WordsRange;
class Hypothesis;

typedef std::vector<Hypothesis*> ArcList;

/** Used to store a state in the beam search
    for the best translation. With its link back to the previous hypothesis
    m_prevHypo, we can trace back to the sentence start to read of the
    (partial) translation to this point.
    
		The expansion of hypotheses is handled in the class Manager, which
    stores active hypothesis in the search in hypothesis stacks.
***/

class Hypothesis
{
	friend std::ostream& operator<<(std::ostream&, const Hypothesis&);
private:
  unsigned char m_compSignature[16]; /**< MD5 checksum for fast comparison */

protected:
	static ObjectPool<Hypothesis> s_objectPool;
	
	const Hypothesis* m_prevHypo; /**< backpointer to previous hypothesis (from which this one was created) */
	const Phrase			&m_targetPhrase; /**< target phrase being created at the current decoding step */
	Phrase const*     m_sourcePhrase; /**< input sentence */
	WordsBitmap				m_sourceCompleted; /**< keeps track of which words have been translated so far */
	//TODO: how to integrate this into confusion network framework; what if
	//it's a confusion network in the end???
	InputType const&  m_sourceInput;
	WordsRange				m_currSourceWordsRange; /**< source word positions of the last phrase that was used to create this hypothesis */
	WordsRange        m_currTargetWordsRange; /**< target word positions of the last phrase that was used to create this hypothesis */
  bool							m_wordDeleted;
	float							m_totalScore;  /**< score so far */
	float							m_futureScore; /**< estimated future cost to translate rest of sentence */
	ScoreComponentCollection2 m_scoreBreakdown; /**< detailed score break-down by components (for instance language model, word penalty, etc) */
	std::vector<LanguageModelSingleFactor::State> m_languageModelStates; /**< relevant history for language model scoring -- used for recombination */
	const Hypothesis 	*m_mainHypo;
	ArcList 					*m_arcList; /**< all arcs that end at the same lattice point as this hypothesis */

	void CalcFutureScore(const SquareMatrix &futureScore);
	//void CalcFutureScore(float futureScore[256][256]);
	void CalcLMScore(const LMList &languageModels);
	void CalcDistortionScore();
	//TODO: add appropriate arguments to score calculator

	void GenerateNGramCompareHash() const;
	mutable size_t _hash;
	mutable bool _hash_computed;

public:
	static ObjectPool<Hypothesis> &GetObjectPool()
	{
		return s_objectPool;
	}

	static unsigned int s_HypothesesCreated; /**< statistics: how many hypotheses were created in total */
	static unsigned int s_numNodes; /**< statistics: how many hypotheses were created in total */
	int m_id; /**< numeric ID of this hypothesis, used for logging */
	
	/** used by initial seeding of the translation process */
	Hypothesis(InputType const& source, const TargetPhrase &emptyTarget);
	/** used when creating a new hypothesis using a translation option (phrase translation) */
	Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt);
	~Hypothesis();
	
	/** return the subclass of Hypothesis most appropriate to the given translation option */
	static Hypothesis* Create(const Hypothesis &prevHypo, const TranslationOption &transOpt);

	/** return the subclass of Hypothesis most appropriate to the given target phrase */
	static Hypothesis* Create(const WordsBitmap &initialCoverage);

	/** return the subclass of Hypothesis most appropriate to the given target phrase */
	static Hypothesis* Create(InputType const& source, const TargetPhrase &emptyTarget);
	
	/** return the subclass of Hypothesis most appropriate to the given translation option */
	Hypothesis* CreateNext(const TranslationOption &transOpt) const;

	/***
	 * if any factors aren't set in our target phrase but are present in transOpt, copy them over
	 * (unless the factors that we do have fail to match the corresponding ones in transOpt,
	 *  in which case presumably there's a programmer's error)
	 * 
	 * return NULL if we aren't compatible with the given option
	 */

	void PrintHypothesis(  const InputType &source, float weightDistortion, float weightWordPenalty) const;

	/** returns target phrase used to create this hypothesis */
	const Phrase &GetTargetPhrase() const
	{
		return m_targetPhrase;
	}

 // void PrintLMScores(const LMList &lmListInitial, const LMList	&lmListEnd) const;
	/** returns input positions covered by the translation option (phrasal translation) used to create this hypothesis */
	inline const WordsRange &GetCurrSourceWordsRange() const
	{
		return m_currSourceWordsRange;
	}
	
	/** returns ouput word positions of the translation option (phrasal translation) used to create this hypothesis */
	inline const WordsRange &GetCurrTargetWordsRange() const
	{
		return m_currTargetWordsRange;
	}
	
	/** output length of the translation option used to create this hypothesis */
	size_t GetCurrTargetLength() const
	{
		return m_currTargetWordsRange.GetWordsCount();
	}

	void ResetScore();

	void CalcScore(const StaticData& staticData, const SquareMatrix &futureScore);

	int GetId() const;


	const Hypothesis* GetPrevHypo() const;

	/** length of the partial translation (from the start of the sentence) */
	inline size_t GetSize() const
	{
		return m_currTargetWordsRange.GetEndPos() + 1;
	}
	/** same as GetTargetPhrase() */
	inline const Phrase &GetPhrase() const
	{
		return m_targetPhrase;
	}
	inline const InputType &GetSourcePhrase() const
	{
		return m_sourceInput;
	}

	std::string GetSourcePhraseStringRep() const;
	std::string GetTargetPhraseStringRep() const;

	// curr - pos is relative from CURRENT hypothesis's starting ind ex
  // (ie, start of sentence would be some negative number, which is
  // not allowed- USE WITH CAUTION)
	inline const FactorArray &GetCurrFactorArray(size_t pos) const
	{
		return m_targetPhrase.GetFactorArray(pos);
	}
	inline const Factor *GetCurrFactor(size_t pos, FactorType factorType) const
	{
		return m_targetPhrase.GetFactor(pos, factorType);
	}
	// recursive - pos is relative from start of sentence
	inline const FactorArray &GetFactorArray(size_t pos) const
	{
		const Hypothesis *hypo = this;
		while (pos < hypo->GetCurrTargetWordsRange().GetStartPos())
		{
			hypo = hypo->GetPrevHypo();
			assert(hypo != NULL);
		}
		return hypo->GetCurrFactorArray(pos - hypo->GetCurrTargetWordsRange().GetStartPos());
	}
	inline const Factor* GetFactor(size_t pos, FactorType factorType) const
	{
		return GetFactorArray(pos)[factorType];
	}

	/***
	 * \return The bitmap of source words we cover
	 */
	inline const WordsBitmap &GetWordsBitmap() const
	{
		return m_sourceCompleted;
	}

	int NGramCompare(const Hypothesis &compare) const;

	inline size_t hash() const
	{
		if (_hash_computed) return _hash;
		GenerateNGramCompareHash();
		return _hash;
	}

	void ToStream(std::ostream& out) const
	{
		if (m_prevHypo != NULL)
		{
			m_prevHypo->ToStream(out);
		}
		out << GetPhrase();
	}

	TO_STRING;

	inline void SetMainHypo(const Hypothesis *hypo)
	{
		m_mainHypo = hypo;
	}
	void AddArc(Hypothesis *loserHypo);

	inline void InitializeArcs()
	{
		if (!m_arcList) return;
		ArcList::iterator iter = m_arcList->begin();
		for (; iter != m_arcList->end() ; ++iter)
		{
			Hypothesis *arc = *iter;
			arc->SetMainHypo(this);
		}
	}

	//! returns a list alternative previous hypotheses (or NULL if n-best support is disabled)
	inline const ArcList* GetArcList() const
	{
		return m_arcList;
	}
	const ScoreComponentCollection2& GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}
	float GetTotalScore() const { return m_totalScore; }
	float GetFutureScore() const { return m_futureScore; }
};


std::ostream& operator<<(std::ostream& out, const Hypothesis& hypothesis);

