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
  Hypothesis(const Manager &mgr, const Moses::WordsBitmap &bitmap);
  virtual ~Hypothesis();

  size_t hash() const;
  bool operator==(const Hypothesis &other) const;

protected:
  const Manager &m_mgr;
  const Moses::WordsBitmap &m_bitmap;

  Moses::FFState **m_ffStates;
};

#endif /* HYPOTHESIS_H_ */
