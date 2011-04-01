#pragma once
// $Id$
/***********************************************************************
 Moses - factored phrase-based, hierarchical and syntactic language decoder
 Copyright (C) 2009 Hieu Hoang

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***********************************************************************/
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "Vocab.h"

namespace Moses
{
class Word;
}

namespace OnDiskPt
{

class Word
{
  friend std::ostream& operator<<(std::ostream&, const Word&);

protected:
  bool m_isNonTerminal;
  std::vector<UINT64> m_factors;

public:
  explicit Word()
  {}

  explicit Word(size_t numFactors, bool isNonTerminal)
  :m_isNonTerminal(isNonTerminal)
  ,m_factors(numFactors)
  {}

  Word(const Word &copy);
  ~Word();


  void CreateFromString(const std::string &inString, Vocab &vocab);
  bool IsNonTerminal() const {
    return m_isNonTerminal;
  }

  size_t WriteToMemory(char *mem) const;
  size_t ReadFromMemory(const char *mem, size_t numFactors);
  size_t ReadFromFile(std::fstream &file, size_t numFactors);

  void SetVocabId(size_t ind, UINT32 vocabId) {
    m_factors[ind] = vocabId;
  }

  Moses::Word *ConvertToMoses(Moses::FactorDirection direction
                              , const std::vector<Moses::FactorType> &outputFactorsVec
                              , const Vocab &vocab) const;

  int Compare(const Word &compare) const;
  bool operator<(const Word &compare) const;
  bool operator==(const Word &compare) const;

};
}

