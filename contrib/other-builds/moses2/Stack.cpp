/*
 * Stack.cpp
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#include "Stack.h"

Stack::Stack() {
	// TODO Auto-generated constructor stub

}

Stack::~Stack() {
	// TODO Auto-generated destructor stub
}

bool Stack::AddPrune(Hypothesis *hypo)
{
  std::pair<iterator, bool> ret = m_hypos.insert(hypo);
  return ret.second;
}

