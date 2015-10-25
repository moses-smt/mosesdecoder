/*
 * Hypothesis.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef HYPOTHESIS_H_
#define HYPOTHESIS_H_

#include <cstddef>
#include "moses/FF/FFState.h"
#include "moses/Bitmap.h"

class Manager;
class TargetPhrase;

class Hypothesis {
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
  { return m_bitmap; }

  const Moses::Range &GetRange() const
  { return m_range; }

protected:
  Manager &m_mgr;
  const TargetPhrase &m_targetPhrase;
  const Moses::Bitmap &m_bitmap;
  const Moses::Range &m_range;
  const Hypothesis *m_prevHypo;

  Moses::FFState **m_ffStates;
};

#endif /* HYPOTHESIS_H_ */
