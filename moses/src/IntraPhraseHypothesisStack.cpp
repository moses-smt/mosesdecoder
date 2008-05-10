
#include <queue>
#include "IntraPhraseHypothesisStack.h"

using namespace std;

IntraPhraseHypothesisStack::IntraPhraseHypothesisStack(size_t maxSize)
:m_maxSize(maxSize)
{
}

IntraPhraseHypothesisStack::~IntraPhraseHypothesisStack()
{
	RemoveAllInColl(m_coll);
}

void IntraPhraseHypothesisStack::AddPrune(IntraPhraseTargetPhrase *phrase)
{
	m_coll.insert(phrase);

	if (m_coll.size() > 10)
	{
		PruneToSize(5);
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


