
#include <queue>
#include "IntraPhraseHypothesisStack.h"
#include "StaticData.h"

using namespace std;

IntraPhraseHypothesisStack::IntraPhraseHypothesisStack(size_t maxSize, bool isLastStack)
:m_maxSize(maxSize)
,m_isLastStack(isLastStack)
{
}

IntraPhraseHypothesisStack::~IntraPhraseHypothesisStack()
{
	RemoveAllInColl(m_coll);
}

void IntraPhraseHypothesisStack::AddPrune(IntraPhraseTargetPhrase *phrase)
{
	if (m_isLastStack)
	{ // need to sync phrases for last stack
		phrase->ClearOverlongTransOpts();
		if (phrase->GetTranslationOptionList().size() == 0)
		{ // not a valid phrase for final stack
			delete phrase;
			return;
		}
	}

	m_coll.insert(phrase);

	if (m_coll.size() > m_maxSize * 2)
	{
		PruneToSize(m_maxSize);
	}
}

void IntraPhraseHypothesisStack::PruneToSize(size_t newSize)
{
	// copy of hypo stack pruning
	if (m_coll.size() > newSize) // ok, if not over the limit
	{
		priority_queue<float> bestScores;

		// push all scores to a heap
		iterator iter;
		float score = 0;
		for (iter = m_coll.begin(); iter != m_coll.end() ; ++iter)
		{
			IntraPhraseTargetPhrase &phrase = **iter;
			score = phrase.GetFutureScore();
			bestScores.push(score);
    }

		// pop the top newSize scores (and ignore them, these are the scores of hyps that will remain)
		//  ensure to never pop beyond heap size
		size_t minNewSizeHeapSize = newSize > bestScores.size() ? bestScores.size() : newSize;
		for (size_t i = 1 ; i < minNewSizeHeapSize ; i++)
			bestScores.pop();

		// and remember the threshold
		float scoreThreshold = bestScores.top();

		// delete all hypos under score threshold
		iter = m_coll.begin();
		while (iter != m_coll.end())
		{
			IntraPhraseTargetPhrase &phrase = **iter;
			float score = phrase.GetFutureScore();
			if (score < scoreThreshold)
			{
				iterator iterRemove = iter++;
				Remove(iterRemove);
			}
			else
			{
				++iter;
			}
		}
	}
}

void IntraPhraseHypothesisStack::RemoveSelectedPhrases()
{
	const StaticData &staticData = StaticData::Instance();
	size_t minSubRangeCount = staticData.GetNumSubRanges()[0]
				,maxSubRangeCount = numeric_limits<size_t>::max();

	// find minimum
	IntraPhraseHypothesisStack::iterator iterStack;

	for (iterStack = begin() ; iterStack != end() ; ++iterStack)
	{
		IntraPhraseTargetPhrase &phrase = **iterStack;
		size_t subRangeCount = phrase.GetSubRangeCount();

		maxSubRangeCount = std::min(maxSubRangeCount, subRangeCount);
	}

	maxSubRangeCount = std::min(maxSubRangeCount, staticData.GetNumSubRanges()[1]);

	// delete any phrase with more than min segments
	iterStack = begin();
	while (iterStack != end())
	{
		IntraPhraseTargetPhrase &phrase = **iterStack;
		size_t subRangeCount = phrase.GetSubRangeCount();

		if (subRangeCount >maxSubRangeCount || subRangeCount < minSubRangeCount)
		{
			IntraPhraseHypothesisStack::iterator iterDelete = iterStack++;
			Remove(iterDelete);
		}
		else
		{
			++iterStack;
		}
	}

}


