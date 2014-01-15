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
#include <iostream>
#include "moses/Util.h"
#include "Phrase.h"
#include "util/exception.hh"

using namespace std;

namespace OnDiskPt
{


void Phrase::AddWord(WordPtr word)
{
  m_words.push_back(word);
}

void Phrase::AddWord(WordPtr word, size_t pos)
{
	UTIL_THROW_IF2(!(pos < m_words.size()),
			"Trying to get word " << pos << " when phrase size is " << m_words.size());
  m_words.insert(m_words.begin() + pos + 1, word);
}

int Phrase::Compare(const Phrase &compare) const
{
  int ret = 0;
  for (size_t pos = 0; pos < GetSize(); ++pos) {
    if (pos >= compare.GetSize()) {
      // we're bigger than the other. Put 1st
      ret = -1;
      break;
    }

    const Word &thisWord = GetWord(pos)
                           ,&compareWord = compare.GetWord(pos);
    int wordRet = thisWord.Compare(compareWord);
    if (wordRet != 0) {
      ret = wordRet;
      break;
    }
  }

  if (ret == 0) {
    assert(compare.GetSize() >= GetSize());
    ret = (compare.GetSize() > GetSize()) ? 1 : 0;
  }
  return ret;
}

//! transitive comparison
bool Phrase::operator<(const Phrase &compare) const
{
  int ret = Compare(compare);
  return ret < 0;
}

bool Phrase::operator>(const Phrase &compare) const
{
  int ret = Compare(compare);
  return ret > 0;
}

bool Phrase::operator==(const Phrase &compare) const
{
  int ret = Compare(compare);
  return ret == 0;
}

void Phrase::DebugPrint(ostream &out, const Vocab &vocab) const
{
  for (size_t pos = 0; pos < GetSize(); ++pos) {
    const Word &word = GetWord(pos);
    word.DebugPrint(out, vocab);
    out << " ";
  }
}

std::ostream& operator<<(std::ostream &out, const Phrase &phrase)
{
  for (size_t pos = 0; pos < phrase.GetSize(); ++pos) {
    const Word &word = phrase.GetWord(pos);
    out << word << " ";
  }

  return out;
}

}

