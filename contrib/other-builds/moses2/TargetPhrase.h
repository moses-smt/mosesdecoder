/*
 * TargetPhrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include "Phrase.h"
#include "MemPool.h"

class Scores;
class Manager;
class System;

class TargetPhrase : public Phrase
{
	  friend std::ostream& operator<<(std::ostream &, const TargetPhrase &);
public:
  static TargetPhrase *CreateFromString(MemPool &pool, const System &system, const std::string &str);
  TargetPhrase(MemPool &pool, const System &system, size_t size);
  virtual ~TargetPhrase();

  Scores &GetScores()
  { return *m_scores; }

  const Scores &GetScores() const
  { return *m_scores; }

protected:
	Scores *m_scores;
};

