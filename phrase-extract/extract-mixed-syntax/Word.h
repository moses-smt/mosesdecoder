/*
 * Word.h
 *
 *  Created on: 18 Feb 2014
 *      Author: s0565741
 */
#pragma once

#include <string>
#include <set>
#include "RuleSymbol.h"

// a terminal
class Word : public RuleSymbol
{
public:
  Word(const Word&); // do not implement
  Word(int pos, const std::string &str);
  virtual ~Word();

  virtual bool IsNonTerm() const {
    return false;
  }

  std::string GetString() const {
    return m_str;
  }

  std::string GetString(int factor) const;

  int GetPos() const {
    return m_pos;
  }

  void AddAlignment(const Word *other);

  const std::set<const Word *> &GetAlignment() const {
    return m_alignment;
  }

  std::set<int> GetAlignmentIndex() const;

  void Output(std::ostream &out) const;
  std::string Debug() const;

  int CompareString(const Word &other) const;

protected:
  int m_pos; // original position in sentence, NOT in lattice
  std::string m_str;
  std::set<const Word *> m_alignment;
};

