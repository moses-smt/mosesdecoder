/*
 * Hypothesis.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef HYPOTHESIS_H_
#define HYPOTHESIS_H_

#include <iostream>
#include <cstddef>
#include "moses/FF/FFState.h"
#include "moses/Bitmap.h"

class Manager;
class TargetPhrase;
class Scores;

class Hypothesis {
	  friend std::ostream& operator<<(std::ostream &, const Hypothesis &);
public:
  Hypothesis(Manager &mgr,
		  const TargetPhrase &tp,
		  const Moses::Range &range,
		  const Moses::Bitmap &bitmap);
  Hypothesis(const Hypothesis &prevHypo,
		  const TargetPhrase &tp,
		  const Moses::Range &pathRange,
		  const Moses::Bitmap &bitmap);
  virtual ~Hypothesis();

  size_t hash() const;
  bool operator==(const Hypothesis &other) const;

  const Moses::Bitmap &GetBitmap() const
  { return m_sourceCompleted; }

  const Moses::Range &GetRange() const
  { return m_range; }

  const Scores &GetScores() const
  { return *m_scores; }

  void OutputToStream(std::ostream &out) const;

protected:
  Manager &m_mgr;
  const TargetPhrase &m_targetPhrase;
  const Moses::Bitmap &m_sourceCompleted;
  const Moses::Range &m_range;
  const Hypothesis *m_prevHypo;

  Moses::FFState **m_ffStates;
  Scores *m_scores;
};

#endif /* HYPOTHESIS_H_ */
