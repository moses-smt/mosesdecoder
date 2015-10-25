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

class Hypothesis {
public:
  Hypothesis(Manager &mgr, const Moses::Bitmap &bitmap, const Moses::Range &range);
  virtual ~Hypothesis();

  size_t hash() const;
  bool operator==(const Hypothesis &other) const;

  const Moses::Bitmap &GetBitmap() const
  { return m_bitmap; }

  const Moses::Range &GetRange() const
  { return m_range; }

protected:
  Manager &m_mgr;
  const Moses::Bitmap &m_bitmap;
  const Moses::Range &m_range;

  Moses::FFState **m_ffStates;
};

#endif /* HYPOTHESIS_H_ */
