/*
 * Hypothesis.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */
#pragma once

#include <iostream>
#include <cstddef>
#include "legacy/FFState.h"
#include "Scores.h"

namespace Moses2
{

class ManagerBase;
class Scores;

class HypothesisBase
{
public:
	  inline ManagerBase &GetManager() const
	  { return *m_mgr; }

	  const Scores &GetScores() const
	  { return *m_scores; }

	  const FFState *GetState(size_t ind) const
	  { return m_ffStates[ind]; }

	  size_t hash(size_t seed = 0) const;
	  bool operator==(const HypothesisBase &other) const;

	  virtual void EvaluateWhenApplied() = 0;


protected:
	  ManagerBase *m_mgr;
	  Scores *m_scores;
	  FFState **m_ffStates;

	  HypothesisBase(MemPool &pool, const System &system);
};


}

