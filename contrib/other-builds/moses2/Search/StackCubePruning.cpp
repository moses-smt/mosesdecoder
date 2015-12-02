/*
 * StackCubePruning.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#include <algorithm>
#include <boost/foreach.hpp>
#include "StackCubePruning.h"
#include "Hypothesis.h"
#include "../Scores.h"

using namespace std;

StackCubePruning::StackCubePruning() {
	// TODO Auto-generated constructor stub

}

StackCubePruning::~StackCubePruning() {
	// TODO Auto-generated destructor stub
}

void StackCubePruning::Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle)
{
	StackAdd added = Add(hypo);

	if (added.toBeDeleted) {
		hypoRecycle.Add(added.toBeDeleted);
	}

}

StackAdd StackCubePruning::Add(const Hypothesis *hypo)
{
  HyposForCubePruning::HypoCoverage key(&hypo->GetBitmap(), hypo->GetRange().GetEndPos());
  _HCType &innerColl = GetColl(key);
  std::pair<_HCType::iterator, bool> addRet = innerColl.insert(hypo);

  // CHECK RECOMBINATION
  if (addRet.second) {
    // equiv hypo doesn't exists
	return StackAdd(true, NULL);
  }
  else {
	  const Hypothesis *hypoExisting = *addRet.first;
	  if (hypo->GetScores().GetTotalScore() > hypoExisting->GetScores().GetTotalScore()) {
		  // incoming hypo is better than the one we have
		  innerColl.erase(addRet.first);

		  // re-add. It better go in
		  std::pair<_HCType::iterator, bool> addRet = innerColl.insert(hypo);
		  assert(addRet.second);

		  return StackAdd(true, const_cast<Hypothesis*>(hypoExisting));
		  /*
		  const_cast<Hypothesis*>(hypo)->Swap(*const_cast<Hypothesis*>(hypoExisting));
		  return StackAdd(true, const_cast<Hypothesis*>(hypo));
		  */
	  }
	  else {
		  // already storing the best hypo. discard incoming hypo
		  return StackAdd(false, const_cast<Hypothesis*>(hypo));
	  }
  }
}

std::vector<const Hypothesis*> StackCubePruning::GetBestHypos(size_t num) const
{
  std::vector<const Hypothesis*> ret;
  BOOST_FOREACH(const Coll::value_type &val, m_coll) {
		const _HCType &hypos = val.second;
		std::copy(hypos.begin(), hypos.end(), ret.end());
  }

  std::vector<const Hypothesis*>::iterator iterMiddle;
  iterMiddle = (num == 0 || ret.size() < num)
			   ? ret.end()
			   : ret.begin()+num;

  std::partial_sort(ret.begin(), iterMiddle, ret.end(),
		  HypothesisFutureScoreOrderer());

  return ret;
}

size_t StackCubePruning::GetHypoSize() const
{
	size_t ret = 0;
	BOOST_FOREACH(const Coll::value_type &val, m_coll) {
		const _HCType &hypos = val.second;
		ret += hypos.size();
	}
	return ret;
}

StackCubePruning::_HCType &StackCubePruning::GetColl(const HyposForCubePruning::HypoCoverage &key)
{
	/*
	_HCType *ret;
	Coll::iterator iter = m_coll.find(key);
	if (iter == m_coll.end()) {
		ret = new _HCType();
		m_coll[key] = ret;
	}
	else {
		ret = iter->second;
	}
	return *ret;
	*/
	return m_coll[key];
}

