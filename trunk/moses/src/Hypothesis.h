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
#include <list>
#include "Phrase.h"
#include "TypeDef.h"
#include "WordsBitmap.h"
#include "Sentence.h"
#include "Phrase.h"
#include "PhraseDictionary.h"
#include "LanguageModel.h"
#include "Arc.h"
#include "LatticeEdge.h"
#include "ScoreComponentCollection.h"

class SquareMatrix;
class PossibleTranslation;

class Hypothesis : public LatticeEdge
{
	friend std::ostream& operator<<(std::ostream&, const Hypothesis&);

protected:
		// phrase in target language. factors completed will be superset 
		//				as that in dictionary
	WordsBitmap				m_sourceCompleted;
	WordsRange				m_currSourceWordsRange, m_currTargetWordsRange;
#ifdef N_BEST
	std::list<Arc*>		m_arcList;
#endif

	bool IsCompatible(const Phrase &phrase) const;
	// none of the factors clash
	void CalcFutureScore(const SquareMatrix &futureScore);
	void CalcLMScore(const LMList		&lmListInitial, const LMList	&lmListEnd);

public:
	Hypothesis(const Hypothesis &copy); 
	// used to create clone
	Hypothesis(const Phrase &phrase);
		// used for initial seeding of trans process
	Hypothesis(const Hypothesis &prevHypo, const PossibleTranslation &possTrans);
		// create next
	~Hypothesis();
	inline Hypothesis *Clone() const
	{
		return new Hypothesis(*this);
	}

	Hypothesis *CreateNext(const PossibleTranslation &possTrans) const;
	Hypothesis *MergeNext(const PossibleTranslation &possTrans) const;

	inline const WordsRange &GetCurrSourceWordsRange() const
	{
		return m_currSourceWordsRange;
	}
	inline size_t GetCurrTargetLength() const
	{
		return m_currTargetWordsRange.GetWordsCount();
	}
	// subsequent translation should only translate this sub-phrase

	void CalcScore(const LMList &lmListInitial
							, const LMList &lmListEnd
							, float weightDistortion
							, float weightWordPenalty
							, const SquareMatrix &futureScore);

	// same as for phrase
	inline size_t GetSize() const
	{
		return m_currTargetWordsRange.GetEndPos() + 1;
	}
	inline const Phrase &GetPhrase() const
	{
		return m_phrase;
	}

	// curr
	inline FactorArray &GetCurrFactorArray(size_t pos)
	{
		return m_phrase.GetFactorArray(pos);
	}
	inline const FactorArray &GetCurrFactorArray(size_t pos) const
	{
		return m_phrase.GetFactorArray(pos);
	}
	inline const Factor *GetCurrFactor(size_t pos, FactorType factorType) const
	{
		return m_phrase.GetFactor(pos, factorType);
	}
	// recursive
	inline const FactorArray &GetFactorArray(size_t pos) const
	{
		if (pos < m_currTargetWordsRange.GetStartPos())
			return m_prevHypo->GetFactorArray(pos);
		return m_phrase.GetFactorArray(pos - m_currTargetWordsRange.GetStartPos());
	}
	inline const Factor *GetFactor(size_t pos, FactorType factorType) const
	{
		if (pos < m_currTargetWordsRange.GetStartPos())
			return m_prevHypo->GetFactor(pos, factorType);
		return m_phrase.GetFactor(pos - m_currTargetWordsRange.GetStartPos(), factorType);
	}

	inline const WordsBitmap &GetWordsBitmap() const
	{
		return m_sourceCompleted;
	}

	void MergeFactors(std::vector< const Word* > mergeWord, size_t idDict, float generationScore, float weight);
		// used in generation processing
		// startPos is usually the start of the last phrase

	int	 NGramCompare(const Hypothesis &compare, size_t nGramSize) const;

	void ToStream(std::ostream& out) const
	{
		if (m_prevHypo != NULL)
		{
			m_prevHypo->ToStream(out);
		}
		out << GetPhrase();
	}

	TO_STRING;

#ifdef N_BEST
	inline void AddArc(Hypothesis &loserHypo)
	{
		Arc *arc = new Arc(loserHypo.m_score
											, loserHypo.GetScoreComponent()
											, loserHypo.GetLMScoreComponent()
											, loserHypo.GetGenerationScoreComponent()
											, loserHypo.GetPhrase()
											, loserHypo.GetPrevHypo());
		m_arcList.push_back(arc);

		// add loser's arcs too
		std::copy(loserHypo.m_arcList.begin(), loserHypo.m_arcList.end()
			, std::inserter(m_arcList, m_arcList.end()));
		loserHypo.m_arcList.clear();
	}
	inline void InitializeArcs()
	{
		std::list<Arc*>::iterator iter;
		for (iter = m_arcList.begin() ; iter != m_arcList.end() ; ++iter)
		{
			Arc *arc = *iter;
			arc->SetMainHypo(*this);
		}
	}
	inline const std::list<Arc*> &GetArcList() const
	{
		return m_arcList;
	}
#endif
};

std::ostream& operator<<(std::ostream& out, const Hypothesis& hypothesis);
