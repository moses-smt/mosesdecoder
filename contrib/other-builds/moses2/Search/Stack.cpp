/*
 * Stack.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#include <algorithm>
#include <boost/foreach.hpp>
#include "Stack.h"
#include "Hypothesis.h"
#include "../Scores.h"

using namespace std;

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

		  return StackAdd(true, const_cast<Hypothesis*>(hypoExisting));
	  }
	  else {
		  // already storing the best hypo. discard incoming hypo
		  return StackAdd(false, const_cast<Hypothesis*>(hypoExisting));
	  }
  }
}

std::vector<const Hypothesis*> Stack::GetBestHypos(size_t num) const
{
  std::vector<const Hypothesis*> ret(m_hypos.begin(), m_hypos.end());

  std::vector<const Hypothesis*>::iterator iterMiddle;
  iterMiddle = (num == 0 || ret.size() < num)
			   ? ret.end()
			   : ret.begin()+num;

  std::partial_sort(ret.begin(), iterMiddle, ret.end(),
		  HypothesisScoreOrderer());

  if (num && ret.size() > num) {
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
