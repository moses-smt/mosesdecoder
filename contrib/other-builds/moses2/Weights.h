/*
 * Weights.h
 *
 *  Created on: 24 Oct 2015
 *      Author: hieu
 */

#ifndef WEIGHTS_H_
#define WEIGHTS_H_

#include <iostream>
#include <vector>
#include "TypeDef.h"

namespace Moses2
{

class FeatureFunctions;

class Weights
{
public:
  Weights();
  virtual ~Weights();
  void Init(const FeatureFunctions &ffs);

  SCORE operator[](size_t ind) const
  {
    return m_weights[ind];
  }

  void Debug(std::ostream &out, const FeatureFunctions &ffs) const;

  void CreateFromString(const FeatureFunctions &ffs, const std::string &line);

protected:
  std::vector<SCORE> m_weights;
};

}

#endif /* WEIGHTS_H_ */
