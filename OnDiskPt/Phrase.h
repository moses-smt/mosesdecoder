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
#include <vector>
#include <iostream>
#include "Word.h"

namespace OnDiskPt
{
class Vocab;

class Phrase
{
  friend std::ostream& operator<<(std::ostream&, const Phrase&);

protected:
  std::vector<Word*>	m_words;

public:
  Phrase()
  {}
  Phrase(const Phrase &copy);
  virtual ~Phrase();

  void AddWord(Word *word);
  void AddWord(Word *word, size_t pos);

  const Word &GetWord(size_t pos) const {
    return *m_words[pos];
  }
  size_t GetSize() const {
    return m_words.size();
  }

  virtual void DebugPrint(std::ostream &out, const Vocab &vocab) const;

  int Compare(const Phrase &compare) const;
  bool operator<(const Phrase &compare) const;
  bool operator>(const Phrase &compare) const;
  bool operator==(const Phrase &compare) const;
};

}
