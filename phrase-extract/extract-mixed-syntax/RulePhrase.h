/*
 * RulePhrase.h
 *
 *  Created on: 26 Feb 2014
 *      Author: hieu
 */

#ifndef RULEPHRASE_H_
#define RULEPHRASE_H_

#include <vector>
#include <cstddef>
#include <iostream>

class RuleSymbol;

// a phrase of terms and non-terms for 1 side of a rule
class RulePhrase
{
public:
  typedef std::vector<const RuleSymbol*> Coll;
  Coll m_coll;

  size_t GetSize() const {
    return m_coll.size();
  }

  void Add(const RuleSymbol *symbol) {
    m_coll.push_back(symbol);
  }

  const RuleSymbol* operator[](size_t index) const {
    return m_coll[index];
  }

  const RuleSymbol* Front() const {
    return m_coll.front();
  }
  const RuleSymbol* Back() const {
    return m_coll.back();
  }

  int Compare(const RulePhrase &other) const;

  void Output(std::ostream &out) const;
  std::string Debug() const;
};

#endif /* RULEPHRASE_H_ */
