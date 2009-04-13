
#include "TranslationOptionList.h"
#include "TranslationOption.h"
#include "Util.h"

TranslationOptionList::TranslationOptionList(const TranslationOptionList &copy)
:m_trainingCount(0)
{
	TranslationOptionList::const_iterator iter;
	for (iter = copy.begin(); iter != copy.end(); ++iter)
	{
		Add(**iter);
	}
}

TranslationOptionList::~TranslationOptionList()
{
	RemoveAllInColl(m_coll);
}

/** helper for pruning */
bool CompareTranslationOption(const TranslationOption *a, const TranslationOption *b)
{
	return a->GetFutureScore() > b->GetFutureScore();
}

void TranslationOptionList::Prune(size_t maxNoTransOptPerCoverage)
{
	if (GetSize() <= maxNoTransOptPerCoverage)
		return;

	// sort in vector
	nth_element(begin(), begin() + maxNoTransOptPerCoverage, end(), CompareTranslationOption);

	for (size_t i = 0 ; i < maxNoTransOptPerCoverage ; ++i)
	{ // reset unique target phrase indexes
		TranslationOption &transOpt = *m_coll[i];
		m_uniqueTargetPhrase[transOpt.GetTargetPhrase()] = i;
	}

	// delete the rest
	for (size_t i = maxNoTransOptPerCoverage ; i < GetSize() ; ++i)
	{
		m_uniqueTargetPhrase.erase(m_coll[i]->GetTargetPhrase());
		delete m_coll[i];
	}
	m_coll.resize(maxNoTransOptPerCoverage);
}

void TranslationOptionList::Add(TranslationOption &transOpt)
{
	const StaticData &staticData = StaticData::Instance();
	OverlapTransOpt overlapTransOpt = staticData.GetOverlapTransOpt();

	bool add;
	switch (overlapTransOpt)
	{
		case KeepAll:
			add = true;
			break;
		case BackoffGraph:
		{ // only process 2nd decode graph if count of 1st is less than threshold
			size_t decodeGraphId = transOpt.GetDecodeGraphId();
			if (decodeGraphId == 0)
			{ // trans opt is for unknown word or the first decode graph
				m_trainingCount += transOpt.GetTrainingCount();
				add = true;
			}
			else
			{ // 2nd graph
				size_t backoffThreshold = staticData.GetBackoffThreshold();
				if (m_trainingCount > backoffThreshold)
				{ // already have enought from prev decode graph, delete trans opt
					add = false;
				}
				else
				{
					add = true;
				}
			}

			break;
		}

		default:
			abort();

	}

	if (add)
	{
		UniqueTargetPhrase::iterator iterUniquePhrase;
		iterUniquePhrase = m_uniqueTargetPhrase.find(transOpt.GetTargetPhrase());
		if (iterUniquePhrase == m_uniqueTargetPhrase.end())
		{ // not there yet. just add
			TranslationOption *newTransOpt = new TranslationOption(transOpt);
			m_coll.push_back(newTransOpt);
			m_uniqueTargetPhrase[transOpt.GetTargetPhrase()] = m_coll.size() - 1;
		}
		else
		{ // already there. clear out lowest scoring trans opt
			size_t transOptInd = iterUniquePhrase->second;
			TranslationOption *origTransOpt = m_coll[transOptInd];
			if (origTransOpt->GetFutureScore() < transOpt.GetFutureScore())
			{ // replace with new
				TranslationOption *newTransOpt = new TranslationOption(transOpt);
				m_coll[transOptInd] = newTransOpt;
				delete origTransOpt;
			}
			else
			{ //keep original

			}
		}
	}
}

std::ostream& operator<<(std::ostream& out, const TranslationOptionList& obj)
{
	TranslationOptionList::const_iterator iter;
	for (iter = obj.begin(); iter !=obj.end(); ++iter) 
	{
		const TranslationOption &transOpt = **iter;
	  out << transOpt << std::endl;
	}
	return out;
}
