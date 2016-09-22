/*
 * HypothesisColl.cpp
 *
 *  Created on: 26 Feb 2016
 *      Author: hieu
 */
#include <iostream>
#include <sstream>
#include <algorithm>
#include <boost/foreach.hpp>
#include "HypothesisColl.h"
#include "ManagerBase.h"
#include "System.h"

using namespace std;

namespace Moses2
{

HypothesisColl::HypothesisColl(const ManagerBase &mgr) :
    		m_coll(MemPoolAllocator<const HypothesisBase*>(mgr.GetPool())), m_sortedHypos(
    				NULL)
{
}

const HypothesisBase *HypothesisColl::GetBestHypo() const
{
	if (GetSize() == 0) {
		return NULL;
	}
	if (m_sortedHypos) {
		return (*m_sortedHypos)[0];
	}

	SCORE bestScore = -std::numeric_limits<SCORE>::infinity();
	const HypothesisBase *bestHypo;
	BOOST_FOREACH(const HypothesisBase *hypo, m_coll) {
		if (hypo->GetFutureScore() > bestScore) {
			bestScore = hypo->GetFutureScore();
			bestHypo = hypo;
		}
	}
	return bestHypo;
}

void HypothesisColl::Add(
		const System &system,
		HypothesisBase *hypo,
		Recycler<HypothesisBase*> &hypoRecycle,
		ArcLists &arcLists)
{
	StackAdd added = Add(hypo);

	size_t nbestSize = system.options.nbest.nbest_size;
	if (nbestSize) {
		arcLists.AddArc(added.added, hypo, added.other);
	}
	else {
		if (!added.added) {
			hypoRecycle.Recycle(hypo);
		}
		else if (added.other) {
			hypoRecycle.Recycle(added.other);
		}
	}

}

StackAdd HypothesisColl::Add(const HypothesisBase *hypo)
{
	std::pair<_HCType::iterator, bool> addRet = m_coll.insert(hypo);

	// CHECK RECOMBINATION
	if (addRet.second) {
		// equiv hypo doesn't exists
		return StackAdd(true, NULL);
	}
	else {
		HypothesisBase *hypoExisting = const_cast<HypothesisBase*>(*addRet.first);
		if (hypo->GetFutureScore() > hypoExisting->GetFutureScore()) {
			// incoming hypo is better than the one we have
			const HypothesisBase * const &hypoExisting1 = *addRet.first;
			const HypothesisBase *&hypoExisting2 =
					const_cast<const HypothesisBase *&>(hypoExisting1);
			hypoExisting2 = hypo;

			return StackAdd(true, hypoExisting);
		}
		else {
			// already storing the best hypo. discard incoming hypo
			return StackAdd(false, hypoExisting);
		}
	}

	assert(false);
}

const Hypotheses &HypothesisColl::GetSortedAndPruneHypos(
		const ManagerBase &mgr,
		ArcLists &arcLists) const
{
	if (m_sortedHypos == NULL) {
		// create sortedHypos first
		MemPool &pool = mgr.GetPool();
		m_sortedHypos = new (pool.Allocate<Hypotheses>()) Hypotheses(pool,
				m_coll.size());

		size_t ind = 0;
		BOOST_FOREACH(const HypothesisBase *hypo, m_coll){
			(*m_sortedHypos)[ind] = hypo;
			++ind;
		}

		SortAndPruneHypos(mgr, arcLists);
	}

	return *m_sortedHypos;
}

const Hypotheses &HypothesisColl::GetSortedAndPrunedHypos() const
{
	UTIL_THROW_IF2(m_sortedHypos == NULL, "m_sortedHypos must be sorted beforehand");
	return *m_sortedHypos;
}

void HypothesisColl::SortAndPruneHypos(const ManagerBase &mgr,
		ArcLists &arcLists) const
{
	size_t stackSize = mgr.system.options.search.stack_size;
	Recycler<HypothesisBase*> &recycler = mgr.GetHypoRecycle();

	/*
   cerr << "UNSORTED hypos: ";
   BOOST_FOREACH(const HypothesisBase *hypo, m_coll) {
	   cerr << hypo << "(" << hypo->GetFutureScore() << ")" << " ";
   }
   cerr << endl;
	 */
	Hypotheses::iterator iterMiddle;
	iterMiddle =
			(stackSize == 0 || m_sortedHypos->size() < stackSize) ?
					m_sortedHypos->end() : m_sortedHypos->begin() + stackSize;

	std::partial_sort(m_sortedHypos->begin(), iterMiddle, m_sortedHypos->end(),
			HypothesisFutureScoreOrderer());

	// prune
	if (stackSize && m_sortedHypos->size() > stackSize) {
		for (size_t i = stackSize; i < m_sortedHypos->size(); ++i) {
			HypothesisBase *hypo = const_cast<HypothesisBase*>((*m_sortedHypos)[i]);
			recycler.Recycle(hypo);

			// delete from arclist
			if (mgr.system.options.nbest.nbest_size) {
				arcLists.Delete(hypo);
			}
		}
		m_sortedHypos->resize(stackSize);
	}

	/*
   cerr << "sorted hypos: ";
   for (size_t i = 0; i < m_sortedHypos->size(); ++i) {
   const HypothesisBase *hypo = (*m_sortedHypos)[i];
   	   cerr << hypo << " ";
   }
   cerr << endl;
	 */
}

void HypothesisColl::Clear()
{
	m_sortedHypos = NULL;
	m_coll.clear();
}

std::string HypothesisColl::Debug(const System &system) const
{
	stringstream out;
	BOOST_FOREACH (const HypothesisBase *hypo, m_coll) {
		out << hypo->Debug(system);
		out << std::endl << std::endl;
	}

	return out.str();
}

} /* namespace Moses2 */
