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
#include "moses/WordsBitmap.h"

class Manager;

class Hypothesis {
public:
  Hypothesis(const Manager &mgr, const Moses::WordsBitmap &bitmap, const Moses::WordsRange &range);
  virtual ~Hypothesis();

  size_t hash() const;
  bool operator==(const Hypothesis &other) const;

  const Moses::WordsBitmap &GetBitmap() const
  { return m_bitmap; }

  const Moses::WordsRange &GetRange() const
  { return m_range; }

protected:
  const Manager &m_mgr;
  const Moses::WordsBitmap &m_bitmap;
  const Moses::WordsRange &m_range;

  Moses::FFState **m_ffStates;
};

#endif /* HYPOTHESIS_H_ */
