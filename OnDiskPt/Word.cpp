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

#include <boost/algorithm/string/predicate.hpp>
#include "moses/Util.h"
#include "Word.h"

#include "util/tokenize_piece.hh"
#include "util/exception.hh"

using namespace std;
using namespace boost::algorithm;

namespace OnDiskPt
{

Word::Word(const Word &copy)
  :m_isNonTerminal(copy.m_isNonTerminal)
  ,m_vocabId(copy.m_vocabId)
{}

Word::~Word()
{}

void Word::CreateFromString(const std::string &inString, Vocab &vocab)
{
  if (starts_with(inString, "[") && ends_with(inString, "]")) {
    // non-term
    m_isNonTerminal = true;
    string str = inString.substr(1, inString.size() - 2);
    m_vocabId = vocab.AddVocabId(str);
  } else {
    m_isNonTerminal = false;
    m_vocabId = vocab.AddVocabId(inString);
  }

}

size_t Word::WriteToMemory(char *mem) const
{
  uint64_t *vocabMem = (uint64_t*) mem;
  vocabMem[0] = m_vocabId;

  size_t size = sizeof(uint64_t);

  // is non-term
  char bNonTerm = (char) m_isNonTerminal;
  mem[size] = bNonTerm;
  ++size;

  return size;
}

size_t Word::ReadFromMemory(const char *mem)
{
  uint64_t *vocabMem = (uint64_t*) mem;
  m_vocabId = vocabMem[0];

  size_t memUsed = sizeof(uint64_t);

  // is non-term
  char bNonTerm;
  bNonTerm = mem[memUsed];
  m_isNonTerminal = (bool) bNonTerm;
  ++memUsed;

  return memUsed;
}

size_t Word::ReadFromFile(std::fstream &file)
{
  const size_t memAlloc = sizeof(uint64_t) + sizeof(char);
  char mem[sizeof(uint64_t) + sizeof(char)];
  file.read(mem, memAlloc);

  size_t memUsed = ReadFromMemory(mem);
  assert(memAlloc == memUsed);

  return memAlloc;
}

int Word::Compare(const Word &compare) const
{
  int ret;

  if (m_isNonTerminal != compare.m_isNonTerminal)
    return m_isNonTerminal ?-1 : 1;

  if (m_vocabId < compare.m_vocabId)
    ret = -1;
  else if (m_vocabId > compare.m_vocabId)
    ret = 1;
  else
    ret = 0;

  return ret;
}

bool Word::operator<(const Word &compare) const
{
  int ret = Compare(compare);
  return ret < 0;
}

bool Word::operator==(const Word &compare) const
{
  int ret = Compare(compare);
  return ret == 0;
}

void Word::DebugPrint(ostream &out, const Vocab &vocab) const
{
  const string &str = vocab.GetString(m_vocabId);
  out << str;
}

std::ostream& operator<<(std::ostream &out, const Word &word)
{
  out << "(";
  out << word.m_vocabId;

  out << (word.m_isNonTerminal ? "n" : "t");
  out << ")";

  return out;
}
}
