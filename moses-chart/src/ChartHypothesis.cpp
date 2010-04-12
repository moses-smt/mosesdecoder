// $Id: PhraseDictionaryNewFormat.h 3045 2010-04-05 13:07:29Z hieuhoang1972 $
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

#include <algorithm>
#include <vector>
#include "ChartHypothesis.h"
#include "QueueEntry.h"
#include "ChartCell.h"
#include "ChartTranslationOption.h"
#include "ChartManager.h"
#include "../../moses/src/TargetPhrase.h"
#include "../../moses/src/Phrase.h"
#include "../../moses/src/StaticData.h"
#include "../../moses/src/DummyScoreProducers.h"
#include "../../moses/src/LMList.h"
#include "../../moses/src/ChartRule.h"

using namespace std;
using namespace Moses;

namespace MosesChart
{
unsigned int Hypothesis::s_HypothesesCreated = 0;

#ifdef USE_HYPO_POOL
	ObjectPool<Hypothesis> Hypothesis::s_objectPool("Hypothesis", 300000);
#endif

Hypothesis::Hypothesis(const QueueEntry &queueEntry, Manager &manager)
:m_targetPhrase(queueEntry.GetTranslationOption().GetChartRule().GetTargetPhrase())
,m_wordsConsumedTargetOrder(queueEntry.GetTranslationOption().GetChartRule().GetWordsConsumedTargetOrder())
,m_id(++s_HypothesesCreated)
,m_currSourceWordsRange(queueEntry.GetTranslationOption().GetSourceWordsRange())
,m_contextPrefix(Output, StaticData::Instance().GetAllLM().GetMaxNGramOrder())
,m_contextSuffix(Output, StaticData::Instance().GetAllLM().GetMaxNGramOrder())
,m_arcList(NULL)
,m_manager(manager)
{
	assert(m_targetPhrase.GetSize() == m_wordsConsumedTargetOrder.size());
	//TRACE_ERR(m_targetPhrase << endl);

	m_numTargetTerminals = m_targetPhrase.GetNumTerminals();

	const std::vector<ChildEntry> &childEntries = queueEntry.GetChildEntries();
	assert(m_prevHypos.empty());
	m_prevHypos.reserve(childEntries.size());
	vector<ChildEntry>::const_iterator iter;
	for (iter = childEntries.begin(); iter != childEntries.end(); ++iter)
	{
		const ChildEntry &childEntry = *iter;
		const Hypothesis *prevHypo = childEntry.GetHypothesis();

		m_numTargetTerminals += prevHypo->GetNumTargetTerminals();

		m_prevHypos.push_back(prevHypo);
	}

	size_t maxNGram = StaticData::Instance().GetAllLM().GetMaxNGramOrder();
	CalcPrefix(m_contextPrefix, maxNGram - 1);
	CalcSuffix(m_contextSuffix, maxNGram - 1);
}

Hypothesis::~Hypothesis()
{
	if (m_arcList)
	{
		ArcList::iterator iter;
		for (iter = m_arcList->begin() ; iter != m_arcList->end() ; ++iter)
		{
			Hypothesis *hypo = *iter;
			Delete(hypo);
		}
		m_arcList->clear();

		delete m_arcList;
	}
}

void Hypothesis::CreateOutputPhrase(Phrase &outPhrase) const
{
	for (size_t pos = 0; pos < m_targetPhrase.GetSize(); ++pos)
	{
		const Word &word = m_targetPhrase.GetWord(pos);
		if (word.IsNonTerminal())
		{ // non-term. fill out with prev hypo
			size_t nonTermInd = m_wordsConsumedTargetOrder[pos];
			const Hypothesis *prevHypo = m_prevHypos[nonTermInd];
			prevHypo->CreateOutputPhrase(outPhrase);
		}
		else
		{
			outPhrase.AddWord(word);
		}
	}
}

Phrase Hypothesis::GetOutputPhrase() const
{
	Phrase outPhrase(Output);
	CreateOutputPhrase(outPhrase);
	return outPhrase;
}

size_t Hypothesis::CalcPrefix(Phrase &ret, size_t size) const
{
	for (size_t pos = 0; pos < m_targetPhrase.GetSize(); ++pos)
	{
		const Word &word = m_targetPhrase.GetWord(pos);
		
		if (word.IsNonTerminal())
		{
			size_t nonTermInd = m_wordsConsumedTargetOrder[pos];
			const Hypothesis *prevHypo = m_prevHypos[nonTermInd];
			size = prevHypo->CalcPrefix(ret, size);
		}
		else
		{
			ret.AddWord(m_targetPhrase.GetWord(pos));
			size--;
		}

		if (size==0)
			break;
	}

	return size;
}

size_t Hypothesis::CalcSuffix(Phrase &ret, size_t size) const
{
	assert(m_contextPrefix.GetSize() <= m_numTargetTerminals);

	if (m_contextPrefix.GetSize() == m_numTargetTerminals)
	{ // small hypo. the prefix will contains the whole hypo
		size_t maxCount = min(m_contextPrefix.GetSize(), size)
					, pos			= m_contextPrefix.GetSize() - 1;

		for (size_t ind = 0; ind < maxCount; ++ind)
		{
			const Word &word = m_contextPrefix.GetWord(pos);
			ret.PrependWord(word);
			--pos;
		}

		size -= maxCount;
		return size;
	}
	else
	{
		for (int pos = (int) m_targetPhrase.GetSize() - 1; pos >= 0 ; --pos)
		{
			const Word &word = m_targetPhrase.GetWord(pos);

			if (word.IsNonTerminal())
			{
				size_t nonTermInd = m_wordsConsumedTargetOrder[pos];
				const Hypothesis *prevHypo = m_prevHypos[nonTermInd];
				size = prevHypo->CalcSuffix(ret, size);
			}
			else
			{
				ret.PrependWord(m_targetPhrase.GetWord(pos));
				size--;
			}

			if (size==0)
				break;
		}

		return size;
	}
}

int Hypothesis::LMContextCompare(const Hypothesis &other) const
{
	// prefix
	if (m_currSourceWordsRange.GetStartPos() > 0)
	{
		int ret = GetPrefix().Compare(other.GetPrefix());
		if (ret != 0)
			return ret;
	}

	// suffix
	size_t inputSize = m_manager.GetSource().GetSize();
	if (m_currSourceWordsRange.GetEndPos() < inputSize - 1)
	{
		int ret = GetSuffix().Compare(other.GetSuffix());
		if (ret != 0)
			return ret;
	}

	// they're the same
	return 0;
}

void Hypothesis::CalcScore()
{
	// total scores from prev hypos
	std::vector<const Hypothesis*>::iterator iter;
	for (iter = m_prevHypos.begin(); iter != m_prevHypos.end(); ++iter)
	{
		const Hypothesis &prevHypo = **iter;
		const ScoreComponentCollection &scoreBreakdown = prevHypo.GetScoreBreakDown();

		m_scoreBreakdown.PlusEquals(scoreBreakdown);
	}

	// translation models & word penalty
	const ScoreComponentCollection &scoreBreakdown = m_targetPhrase.GetScoreBreakdown();
	m_scoreBreakdown.PlusEquals(scoreBreakdown);

	CalcLMScore();

	m_totalScore	= m_scoreBreakdown.GetWeightedScore();
}

void Hypothesis::CalcLMScore()
{
	assert(m_lmNGram.GetWeightedScore() == 0);

	m_scoreBreakdown.ZeroAllLM();

	const LMList &lmList = StaticData::Instance().GetAllLM();
	Phrase outPhrase(Output); // = GetOutputPhrase();
	bool calcNow = false, firstPhrase = true;

	for (size_t targetPhrasePos = 0; targetPhrasePos < m_targetPhrase.GetSize(); ++targetPhrasePos)
	{
		const Word &targetWord = m_targetPhrase.GetWord(targetPhrasePos);
		if (!targetWord.IsNonTerminal())
		{ // just a word, add to phrase for lm scoring
			outPhrase.AddWord(targetWord);
		}
		else
		{
			size_t nonTermInd = m_wordsConsumedTargetOrder[targetPhrasePos];
			const Hypothesis *prevHypo = m_prevHypos[nonTermInd];
			size_t numTargetTerminals = prevHypo->GetNumTargetTerminals();

			if (numTargetTerminals >= lmList.GetMaxNGramOrder() - 1)
			{ // large hypo (for trigram lm, another hypo equal or over 2 words). just take the prefix & suffix
				m_lmNGram.PlusEqualsAllLM(prevHypo->m_lmNGram);

				// calc & add overlapping lm scores
				// prefix
				outPhrase.Append(prevHypo->GetPrefix());
				calcNow = true;
			}
			else
			{ // small hypo (for trigram lm, 1-word hypos).
				// add target phrase to temp phrase and continue, but don't score yet
				outPhrase.Append(prevHypo->GetPrefix());
			}

			if (calcNow)
			{
				if (targetPhrasePos == 0 && numTargetTerminals >= lmList.GetMaxNGramOrder() - 1)
				{ // get from other prev hypo. faster
					m_lmPrefix.Assign(prevHypo->m_lmPrefix);
					m_lmNGram.Assign(prevHypo->m_lmNGram);
				}
				else
				{ // calc
					lmList.CalcAllLMScores(outPhrase
													, m_lmNGram
													, (firstPhrase) ? &m_lmPrefix : NULL);
				}

				// create new phrase from suffix. score later when appended with next words
				outPhrase.Clear();
				outPhrase.Append(prevHypo->GetSuffix());

				firstPhrase = false;
				calcNow = false;
			}
		} // if (!targetWord.IsNonTerminal())
	} // for (size_t targetPhrasePos

	lmList.CalcAllLMScores(outPhrase
									, m_lmNGram
									, (firstPhrase) ? &m_lmPrefix : NULL);

	m_scoreBreakdown.PlusEqualsAllLM(m_lmPrefix);
	m_scoreBreakdown.PlusEqualsAllLM(m_lmNGram);

/*
	// lazy way. keep for comparison
	Phrase outPhrase = GetOutputPhrase();
//	cerr << outPhrase << " ";

	float retFullScore, retNGramScore;
	StaticData::Instance().GetAllLM().CalcScore(outPhrase
																						, retFullScore
																						, retNGramScore
																						, m_scoreBreakdown
																						, &m_lmNGram
																						, false);
*/
}

void Hypothesis::AddArc(Hypothesis *loserHypo)
{
	if (!m_arcList) {
		if (loserHypo->m_arcList)  // we don't have an arcList, but loser does
		{
			this->m_arcList = loserHypo->m_arcList;  // take ownership, we'll delete
			loserHypo->m_arcList = 0;                // prevent a double deletion
		}
		else
			{ this->m_arcList = new ArcList(); }
	} else {
		if (loserHypo->m_arcList) {  // both have an arc list: merge. delete loser
			size_t my_size = m_arcList->size();
			size_t add_size = loserHypo->m_arcList->size();
			this->m_arcList->resize(my_size + add_size, 0);
			std::memcpy(&(*m_arcList)[0] + my_size, &(*loserHypo->m_arcList)[0], add_size * sizeof(Hypothesis *));
			delete loserHypo->m_arcList;
			loserHypo->m_arcList = 0;
		} else { // loserHypo doesn't have any arcs
		  // DO NOTHING
		}
	}
	m_arcList->push_back(loserHypo);
}

// sorting helper
struct CompareChartHypothesisTotalScore
{
	bool operator()(const Hypothesis* hypo1, const Hypothesis* hypo2) const
	{
		return hypo1->GetTotalScore() > hypo2->GetTotalScore();
	}
};

void Hypothesis::CleanupArcList()
{
	// point this hypo's main hypo to itself
	m_winningHypo = this;

	if (!m_arcList) return;

	/* keep only number of arcs we need to create all n-best paths.
	 * However, may not be enough if only unique candidates are needed,
	 * so we'll keep all of arc list if nedd distinct n-best list
	 */
	const StaticData &staticData = StaticData::Instance();
	size_t nBestSize = staticData.GetNBestSize();
	bool distinctNBest = staticData.GetDistinctNBest() || staticData.UseMBR() || staticData.GetOutputSearchGraph();

	if (!distinctNBest && m_arcList->size() > nBestSize)
	{ // prune arc list only if there too many arcs
		nth_element(m_arcList->begin()
							, m_arcList->begin() + nBestSize - 1
							, m_arcList->end()
							, CompareChartHypothesisTotalScore());
		
		// delete bad ones
		ArcList::iterator iter;
		for (iter = m_arcList->begin() + nBestSize ; iter != m_arcList->end() ; ++iter)
		{
			Hypothesis *arc = *iter;
			Hypothesis::Delete(arc);
		}
		m_arcList->erase(m_arcList->begin() + nBestSize
										, m_arcList->end());
	}

	// set all arc's main hypo variable to this hypo
	ArcList::iterator iter = m_arcList->begin();
	for (; iter != m_arcList->end() ; ++iter)
	{
		Hypothesis *arc = *iter;
		arc->SetWinningHypo(this);
	}
  
  //cerr << m_arcList->size() << " ";
}

void Hypothesis::SetWinningHypo(const Hypothesis *hypo)
{
  m_winningHypo = hypo;
  
  // never gonna use to recombine. clear prefix & suffix phrases to save mem
  m_contextPrefix.Clear();
  m_contextSuffix.Clear();
}

TO_STRING_BODY(Hypothesis)

// friend
ostream& operator<<(ostream& out, const Hypothesis& hypo)
{
	//Phrase outPhrase(Output);
	//hypo.CreateOutputPhrase(outPhrase);

	// words bitmap
	out << " " << hypo.GetId()
			<< " " << hypo.m_targetPhrase
			//<< " " << outPhrase
			<< " " << hypo.GetCurrSourceRange()
			//<< " " << hypo.m_currSourceWordsRange
			<< " " << hypo.GetTotalScore()
			<< " " << hypo.GetScoreBreakDown();
	
	HypoList::const_iterator iter;
	for (iter = hypo.GetPrevHypos().begin(); iter != hypo.GetPrevHypos().end(); ++iter)
	{
		const Hypothesis &prevHypo = **iter;
		out << " " << prevHypo.GetId();
	}

	//out << endl;

	return out;
}

}

