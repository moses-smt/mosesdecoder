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

class RuleSymbol;

class RulePhrase
{
public:
  typedef std::vector<const RuleSymbol*> Coll;
  Coll m_coll;

  size_t GetSize() const
  { return m_coll.size(); }

  void Add(const RuleSymbol *symbol)
  {
	  m_coll.push_back(symbol);
  }

  const RuleSymbol* operator[](size_t index) const {
    return m_coll[index];
  }

  int Compare(const RulePhrase &other) const;

};

#endif /* RULEPHRASE_H_ */
