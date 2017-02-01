/*
 * Word.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include <iostream>
#include "TypeDef.h"
#include "legacy/Factor.h"
#include "legacy/FactorCollection.h"

namespace Moses2
{

class Word
{
public:
  explicit Word();
  Word(const Word &copy);

  virtual ~Word();

  void CreateFromString(FactorCollection &vocab, const System &system,
                        const std::string &str);

  virtual size_t hash() const;
  virtual size_t hash(const std::vector<FactorType> &factors) const;

  int Compare(const Word &compare) const;

  virtual bool operator==(const Word &compare) const {
    int cmp = Compare(compare);
    return cmp == 0;
  }

  virtual bool operator!=(const Word &compare) const {
    return !((*this) == compare);
  }

  virtual bool operator<(const Word &compare) const;

  const Factor* operator[](size_t ind) const {
    return m_factors[ind];
  }

  const Factor*& operator[](size_t ind) {
    return m_factors[ind];
  }

  virtual void OutputToStream(const System &system, std::ostream &out) const;
  virtual std::string Debug(const System &system) const;

  std::string GetString(const FactorList &factorTypes) const;
protected:
  const Factor *m_factors[MAX_NUM_FACTORS];

};

}

