/*
 * NonTerm.h
 *
 *  Created on: 22 Feb 2014
 *      Author: hieu
 */
#pragma once
#include <string>
#include "RuleSymbol.h"
#include "moses/TypeDef.h"

class ConsistentPhrase;
class Parameter;

class NonTerm : public RuleSymbol
{
public:

  NonTerm(const ConsistentPhrase &consistentPhrase,
          const std::string &source,
          const std::string &target);
  virtual ~NonTerm();

  const ConsistentPhrase &GetConsistentPhrase() const {
    return *m_consistentPhrase;
  }

  int GetWidth(Moses::FactorDirection direction) const;

  virtual bool IsNonTerm() const {
    return true;
  }

  std::string GetString() const {
    return m_source + m_target;
  }

  virtual std::string Debug() const;
  virtual void Output(std::ostream &out) const;
  void Output(std::ostream &out, Moses::FactorDirection direction) const;

  const std::string &GetLabel(Moses::FactorDirection direction) const;
  bool IsHiero(Moses::FactorDirection direction, const Parameter &params) const;
  bool IsHiero(const Parameter &params) const;

protected:
  const ConsistentPhrase *m_consistentPhrase;
  std::string m_source, m_target;
};

