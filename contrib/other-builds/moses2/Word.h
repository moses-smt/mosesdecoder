/*
 * Word.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "TypeDef.h"
#include "moses/Factor.h"
#include "moses/FactorCollection.h"

class Word {
public:
  Word();
  virtual ~Word();

  void CreateFromString(Moses::FactorCollection &vocab, const std::string &str);

  size_t hash() const;
  bool operator==(const Word &compare) const;

protected:
  const Moses::Factor *m_factors[MAX_NUM_FACTORS];

};

