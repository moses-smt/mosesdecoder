/*
 * Stack.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#include "Stack.h"
#include "Hypothesis.h"
#include "../Scores.h"

Stack::Stack() {
	// TODO Auto-generated constructor stub

}

Stack::~Stack() {
	// TODO Auto-generated destructor stub
}

StackAdd Stack::Add(const Hypothesis *hypo)
{
  std::pair<iterator, bool> addRet = m_hypos.insert(hypo);
  if (addRet.second) {
    // equiv hypo doesn't exists
	return StackAdd(true, NULL);
  }
  else {
	  const Hypothesis *hypoExisting = *addRet.first;
	  if (hypo->GetScores().GetTotalScore() > hypoExisting->GetScores().GetTotalScore()) {
		  // incoming hypo is better than the one we have
		  m_hypos.erase(addRet.first);

		  // re-add. It better go in
		  std::pair<iterator, bool> addRet = m_hypos.insert(hypo);
		  assert(addRet.second);

		  return StackAdd(true, hypoExisting);
	  }
	  else {
		  // already storing the best hypo. discard incoming hypo
		  return StackAdd(false, hypoExisting);
	  }
  }
}

std::vector<const Hypothesis*> Stack::GetBestHypos(size_t num) const
{
	std::vector<const Hypothesis*> ret(m_hypos.begin(), m_hypos.end());

	if (ret.size() > num) {
		NTH_ELEMENT3(ret.begin(),
				ret.begin() + num,
				ret.end());
		ret.resize(num);
	}

    return ret;
}

std::vector<const Hypothesis*> Stack::GetSortedHypos() const
{
	std::vector<const Hypothesis*> ret(m_hypos.begin(), m_hypos.end());
	std::sort(ret.begin(), ret.end(), HypothesisScoreOrderer());
	return ret;
}
