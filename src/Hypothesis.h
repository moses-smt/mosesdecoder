// $Id$
// vim:tabstop=2

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
#include "PhraseDictionaryMemory.h"
#include "GenerationDictionary.h"
#include "LanguageModelSingleFactor.h"
#include "ScoreComponentCollection.h"
#include "LexicalReordering.h"
#include "InputType.h"
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
	ScoreComponentCollection m_scoreBreakdown; /**< detailed score break-down by components (for instance language model, word penalty, etc) */
	std::vector<LanguageModelSingleFactor::State> m_languageModelStates; /**< relevant history for language model scoring -- used for recombination */
	const Hypothesis 	*m_winningHypo;
	ArcList 					*m_arcList; /**< all arcs that end at the same lattice point as this hypothesis */

	int m_id; /**< numeric ID of this hypothesis, used for logging */
	std::vector<std::vector<unsigned int> >* m_lmstats; /** Statistics: (see IsComputeLMBackoffStats() in StaticData.h */
	static unsigned int s_HypothesesCreated; // Statistics: how many hypotheses were created in total	

	void CalcFutureScore(const SquareMatrix &futureScore);
	//void CalcFutureScore(float futureScore[256][256]);
	void CalcLMScore(const LMList &languageModels);
	void CalcDistortionScore();
	//TODO: add appropriate arguments to score calculator

	/** used by initial seeding of the translation process */
	Hypothesis(InputType const& source, const TargetPhrase &emptyTarget);
	/** used when creating a new hypothesis using a translation option (phrase translation) */
	Hypothesis(const Hypothesis &prevHypo, const TranslationOption &transOpt);

public:
	static ObjectPool<Hypothesis> &GetObjectPool()
	{
		return s_objectPool;
	}

	~Hypothesis();
	
	/** return the subclass of Hypothesis most appropriate to the given translation option */
	static Hypothesis* Create(const Hypothesis &prevHypo, const TranslationOption &transOpt);

	static Hypothesis* Create(const WordsBitmap &initialCoverage);

	/** return the subclass of Hypothesis most appropriate to the given target phrase */
	static Hypothesis* Create(InputType const& source, const TargetPhrase &emptyTarget);
	
	/** return the subclass of Hypothesis most appropriate to the given translation option */
	Hypothesis* CreateNext(const TranslationOption &transOpt) const;

	void PrintHypothesis(  const InputType &source, float weightDistortion, float weightWordPenalty) const;

	/** return target phrase used to create this hypothesis */
	const Phrase &GetCurrTargetPhrase() const
	{
		return m_targetPhrase;
	}

 // void PrintLMScores(const LMList &lmListInitial, const LMList	&lmListEnd) const;
 
	/** return input positions covered by the translation option (phrasal translation) used to create this hypothesis */
	inline const WordsRange &GetCurrSourceWordsRange() const
	{
		return m_currSourceWordsRange;
	}
	
	inline const WordsRange &GetCurrTargetWordsRange() const
	{
		return m_currTargetWordsRange;
	}
	
	/** output length of the translation option used to create this hypothesis */
	size_t GetCurrTargetLength() const
	{
		return m_currTargetWordsRange.GetNumWordsCovered();
	}

	void ResetScore();

	void CalcScore(const StaticData& staticData, const SquareMatrix &futureScore);

	int GetId()const
	{
		return m_id;
	}

	const Hypothesis* GetPrevHypo() const;

	/** length of the partial translation (from the start of the sentence) */
	inline size_t GetSize() const
	{
		return m_currTargetWordsRange.GetEndPos() + 1;
	}

	inline const InputType &GetSourcePhrase() const
	{
		return m_sourceInput;
	}

	std::string GetSourcePhraseStringRep(const vector<FactorType> factorsToPrint) const;
	std::string GetTargetPhraseStringRep(const vector<FactorType> factorsToPrint) const;
	std::string GetSourcePhraseStringRep() const;
	std::string GetTargetPhraseStringRep() const;

	/** curr - pos is relative from CURRENT hypothesis's starting index
	 * (ie, start of sentence would be some negative number, which is
	 * not allowed- USE WITH CAUTION) */
	inline const Word &GetCurrWord(size_t pos) const
	{
		return m_targetPhrase.GetWord(pos);
	}
	inline const Factor *GetCurrFactor(size_t pos, FactorType factorType) const
	{
		return m_targetPhrase.GetFactor(pos, factorType);
	}
	/** recursive - pos is relative from start of sentence */
	inline const Word &GetWord(size_t pos) const
	{
		const Hypothesis *hypo = this;
		while (pos < hypo->GetCurrTargetWordsRange().GetStartPos())
		{
			hypo = hypo->GetPrevHypo();
			assert(hypo != NULL);
		}
		return hypo->GetCurrWord(pos - hypo->GetCurrTargetWordsRange().GetStartPos());
	}
	inline const Factor* GetFactor(size_t pos, FactorType factorType) const
	{
		return GetWord(pos)[factorType];
	}

	/***
	 * \return The bitmap of source words we cover
	 */
	inline const WordsBitmap &GetWordsBitmap() const
	{
		return m_sourceCompleted;
	}

	int NGramCompare(const Hypothesis &compare) const;

	//	inline size_t hash() const
	//	{
	//		if (_hash_computed) return _hash;
	//		GenerateNGramCompareHash();
	//		return _hash;
	//	}

	void ToStream(std::ostream& out) const
	{
		if (m_prevHypo != NULL)
		{
			m_prevHypo->ToStream(out);
		}
		out << GetTargetPhrase();
	}

	TO_STRING();

	inline void SetWinningHypo(const Hypothesis *hypo)
	{
		m_winningHypo = hypo;
	}
	inline const Hypothesis *GetWinningHypo() const
	{
		return m_winningHypo;
	}
	
	void AddArc(Hypothesis *loserHypo);
	void CleanupArcList();

	//! returns a list alternative previous hypotheses (or NULL if n-best support is disabled)
	inline const ArcList* GetArcList() const
	{
		return m_arcList;
	}
	const ScoreComponentCollection& GetScoreBreakdown() const
	{
		return m_scoreBreakdown;
	}
	float GetTotalScore() const { return m_totalScore; }
	float GetFutureScore() const { return m_futureScore; }

	std::vector<std::vector<unsigned int> > *GetLMStats() const
	{
		return m_lmstats;
	}

	static unsigned int GetHypothesesCreated()
	{
		return s_HypothesesCreated;
	}
};

std::ostream& operator<<(std::ostream& out, const Hypothesis& hypothesis);

// sorting helper
struct CompareHypothesisTotalScore
{
	bool operator()(const Hypothesis* hypo1, const Hypothesis* hypo2) const
	{
		return hypo1->GetTotalScore() > hypo2->GetTotalScore();
	}
};

