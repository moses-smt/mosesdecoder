// $Id$
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
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
#include "../../moses/src/Util.h"
#include "../../moses/src/WordsRange.h"
#include "../../moses/src/ScoreComponentCollection.h"
#include "../../moses/src/Phrase.h"
#include "../../moses/src/ChartRule.h"
#include "../../moses/src/ObjectPool.h"

namespace MosesChart
{
class QueueEntry;
class Hypothesis;
class Manager;
	
typedef std::vector<Hypothesis*> ArcList;

class Hypothesis
{
	friend std::ostream& operator<<(std::ostream&, const Hypothesis&);

protected:

#ifdef USE_HYPO_POOL
		static ObjectPool<Hypothesis> s_objectPool;
#endif

	static unsigned int s_HypothesesCreated;

	int m_id; /**< numeric ID of this hypothesis, used for logging */
	const Moses::ChartRule &m_transOpt;

	Moses::Phrase m_contextPrefix, m_contextSuffix;
	const std::vector<size_t> &m_wordsConsumedTargetOrder; // same size as target phrase ?
	Moses::WordsRange					m_currSourceWordsRange;
	Moses::ScoreComponentCollection m_scoreBreakdown /*! detailed score break-down by components (for instance language model, word penalty, etc) */
																	,m_lmNGram
																	,m_lmPrefix;
	float m_totalScore;
	size_t m_numTargetTerminals;

	ArcList 					*m_arcList; /*! all arcs that end at the same trellis point as this hypothesis */
	const Hypothesis 	*m_winningHypo;

	std::vector<const Hypothesis*> m_prevHypos;

	Manager& m_manager;
	
	size_t CalcPrefix(Moses::Phrase &ret, size_t size) const;
	size_t CalcSuffix(Moses::Phrase &ret, size_t size) const;

	void CalcLMScore();

	Hypothesis(); // not implemented
	Hypothesis(const Hypothesis &copy); // not implemented

public:
	static void ResetHypoCount()
	{ s_HypothesesCreated = 0; }
	static unsigned int GetHypoCount()
	{ return s_HypothesesCreated; }

#ifdef USE_HYPO_POOL
	void *operator new(size_t num_bytes)
	{
		void *ptr = s_objectPool.getPtr();
		return ptr;
	}

	static void Delete(Hypothesis *hypo)
	{
		s_objectPool.freeObject(hypo);
	}
#else
	static void Delete(Hypothesis *hypo)
	{
		delete hypo;
	}
#endif

	explicit Hypothesis(const QueueEntry &queueEntry, Manager &manager);
	~Hypothesis();

	int GetId()const
	{	return m_id;}
	const Moses::ChartRule &GetTranslationOption()const
	{ return m_transOpt; }
	const Moses::TargetPhrase &GetCurrTargetPhrase()const
	{ return m_transOpt.GetTargetPhrase(); }
	const Moses::WordsRange &GetCurrSourceRange()const
	{ return m_currSourceWordsRange; }
	inline const ArcList* GetArcList() const
	{
		return m_arcList;
	}

	void CreateOutputPhrase(Moses::Phrase &outPhrase) const;
	Moses::Phrase GetOutputPhrase() const;

	int LMContextCompare(const Hypothesis &other) const;

	const Moses::Phrase &GetPrefix() const
	{ return m_contextPrefix; }
	const Moses::Phrase &GetSuffix() const
	{ return m_contextSuffix; }

	void CalcScore();

	void AddArc(Hypothesis *loserHypo);
	void CleanupArcList();
	void SetWinningHypo(const Hypothesis *hypo);

	const Moses::ScoreComponentCollection &GetScoreBreakdown() const
	{ return m_scoreBreakdown; }
	float GetTotalScore() const 
	{ return m_totalScore; }

	const std::vector<const Hypothesis*> &GetPrevHypos() const
	{ return m_prevHypos; }

	size_t GetWordsConsumedTargetOrder(size_t pos) const
	{ 
		assert(pos < m_wordsConsumedTargetOrder.size());
		return m_wordsConsumedTargetOrder[pos]; 
	}

	const Moses::Word &GetTargetLHS() const
	{ return GetCurrTargetPhrase().GetTargetLHS(); }

	size_t GetNumTargetTerminals() const
	{ 
		return m_numTargetTerminals;
	}

	TO_STRING();

}; // class Hypothesis

}

