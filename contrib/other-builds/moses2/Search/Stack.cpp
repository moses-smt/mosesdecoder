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

StackAdd Stack::Add(Hypothesis *hypo)
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
		  hypo->Swap(const_cast<Hypothesis&>(*hypoExisting));

		  return StackAdd(true, hypo);
	  }
	  else {
		  // already storing the best hypo. discard incoming hypo
		  return StackAdd(false, hypo);
	  }
  }
}

std::vector<const Hypothesis*> Stack::GetBestHyposAndPrune(size_t num, Recycler<Hypothesis*> &recycler) const
{
  std::vector<const Hypothesis*> ret = GetBestHypos(num);
  if (num && ret.size() > num) {
	  for (size_t i = num; i < ret.size(); ++i) {
		  Hypothesis *hypo = const_cast<Hypothesis*>(ret[i]);
		  recycler.push(hypo);
	  }
	  ret.resize(num);
  }
  return ret;
}

std::vector<const Hypothesis*> Stack::GetBestHypos(size_t num) const
{
  std::vector<const Hypothesis*> ret(m_hypos.begin(), m_hypos.end());

  std::vector<const Hypothesis*>::iterator iterMiddle;
  iterMiddle = (num == 0 || ret.size() < num)
			   ? ret.end()
			   : ret.begin()+num;

  std::partial_sort(ret.begin(), iterMiddle, ret.end(),
		  HypothesisFutureScoreOrderer());

  return ret;
}
