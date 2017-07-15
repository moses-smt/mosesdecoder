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
#include <boost/shared_ptr.hpp>
#include "Vocab.h"

namespace Moses
{
class Word;
}

namespace OnDiskPt
{
class Vocab;

/* A wrapper around a vocab id, and a boolean indicating whther it is a term or non-term.
 * Factors can be represented by using a vocab string with | character, eg go|VB
 */
class Word
{
  friend std::ostream& operator<<(std::ostream&, const Word&);

private:
  bool m_isNonTerminal;
  uint64_t m_vocabId;

public:
  explicit Word() {
  }

  explicit Word(bool isNonTerminal)
    :m_isNonTerminal(isNonTerminal)
    ,m_vocabId(0) {
  }

  Word(const Word &copy);
  ~Word();


  void CreateFromString(const std::string &inString, Vocab &vocab);
  bool IsNonTerminal() const {
    return m_isNonTerminal;
  }

  size_t WriteToMemory(char *mem) const;
  size_t ReadFromMemory(const char *mem);
  size_t ReadFromFile(std::fstream &file);

  uint64_t GetVocabId() const {
    return m_vocabId;
  }

  void SetVocabId(uint64_t vocabId) {
    m_vocabId = vocabId;
  }

  void DebugPrint(std::ostream &out, const Vocab &vocab) const;
  inline const std::string &GetString(const Vocab &vocab) const {
    return vocab.GetString(m_vocabId);
  }

  int Compare(const Word &compare) const;
  bool operator<(const Word &compare) const;
  bool operator==(const Word &compare) const;

};

typedef boost::shared_ptr<Word> WordPtr;
}

