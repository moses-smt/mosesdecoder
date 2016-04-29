/*
 * Word.h
 *
 *  Created on: 23 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "../Word.h"

namespace Moses2
{
namespace SCFG
{

class Word: public Moses2::Word
{
public:
  bool isNonTerminal;

  Word() {}
  Word(const Word &copy);

  void CreateFromString(FactorCollection &vocab,
      const System &system,
      const std::string &str);

  bool operator==(const SCFG::Word &compare) const
  {
    int cmp = Word::Compare(compare);
    if (cmp == 0 && isNonTerminal == compare.isNonTerminal) {
      return true;
    }
    else {
      return false;
    }
  }

  size_t hash() const;
  virtual void Debug(std::ostream &out) const;

protected:
};

inline size_t hash_value(const SCFG::Word &word)
{ return word.hash(); }

}
}

