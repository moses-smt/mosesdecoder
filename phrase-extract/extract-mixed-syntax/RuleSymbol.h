/*
 * RuleSymbol.h
 *
 *  Created on: 21 Feb 2014
 *      Author: hieu
 */

#ifndef RULESYMBOL_H_
#define RULESYMBOL_H_

#include <iostream>
#include <string>

// base class - terminal or non-term
class RuleSymbol
{
public:
  RuleSymbol();
  virtual ~RuleSymbol();

  virtual bool IsNonTerm() const = 0;

  virtual std::string Debug() const = 0;
  virtual void Output(std::ostream &out) const = 0;

  virtual std::string GetString() const = 0;

  int Compare(const RuleSymbol &other) const;

};

#endif /* RULESYMBOL_H_ */
