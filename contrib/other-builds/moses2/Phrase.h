/*
 * PhraseImpl.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <cstddef>
#include <string>
#include <iostream>
#include "Word.h"
#include "MemPool.h"
#include "legacy/FactorCollection.h"

namespace Moses2
{

class SubPhrase;
class Scores;

class Phrase
{
	  friend std::ostream& operator<<(std::ostream &, const Phrase &);
public:
  virtual const Word& operator[](size_t pos) const = 0;
  virtual size_t GetSize() const = 0;
  virtual size_t hash() const;
  virtual bool operator==(const Phrase &compare) const;
  virtual bool operator!=(const Phrase &compare) const
  {
		return !( (*this) == compare );
  }
  virtual std::string GetString(const FactorList &factorTypes) const;
  virtual SubPhrase GetSubPhrase(size_t start, size_t end) const = 0;

};
////////////////////////////////////////////////////////////////////////
class TPBase : public Phrase
{
public:
  Scores &GetScores()
  { return *m_scores; }

  const Scores &GetScores() const
  { return *m_scores; }

  SCORE GetFutureScore() const;

  void SetEstimatedScore(const SCORE &value)
  { m_estimatedScore = value; }

  SCORE *GetScoresProperty(int propertyInd) const;
protected:
	Scores *m_scores;
	SCORE m_estimatedScore;

};

////////////////////////////////////////////////////////////////////////
class PhraseOrdererLexical
{
public:
  bool operator()(const Phrase &a, const Phrase &b) const {
	size_t minSize = std::min(a.GetSize(), b.GetSize());
	for (size_t i = 0; i < minSize; ++i) {
		const Word &aWord = a[i];
		const Word &bWord = b[i];
		int cmp = aWord.Compare(bWord);
		//std::cerr << "WORD: " << aWord << " ||| " << bWord << " ||| " << lessThan << std::endl;
		if (cmp) {
			return (cmp < 0);
		}
	}
	return a.GetSize() < b.GetSize();
  }
};

}

