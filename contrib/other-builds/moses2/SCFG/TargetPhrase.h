/*
 * TargetPhrase.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "PhraseImpl.h"

namespace Moses2
{
class Scores;
class Manager;
class System;
class PhraseTable;

namespace SCFG
{

class TargetPhrase : public PhraseImpl
{
	  friend std::ostream& operator<<(std::ostream &, const TargetPhrase &);
public:
	  mutable void **ffData;
	  SCORE *scoreProperties;
	  const PhraseTable &pt;

	  static TargetPhrase *CreateFromString(MemPool &pool, const PhraseTable &pt, const System &system, const std::string &str);

	  TargetPhrase(MemPool &pool, const PhraseTable &pt, const System &system, size_t size);

	  Scores &GetScores()
	  { return *m_scores; }

	  const Scores &GetScores() const
	  { return *m_scores; }

protected:
	  Scores *m_scores;
	  Word m_lhs;

};

}

}

