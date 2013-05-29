/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2011- University of Edinburgh

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

#include "util/file_piece.hh"
#include "util/tokenize_piece.hh"

#include "ScoreArray.h"
#include "ScoreDataIterator.h"

using namespace std;
using namespace util;

namespace MosesTuning
{


ScoreDataIterator::ScoreDataIterator() {}

ScoreDataIterator::ScoreDataIterator(const string& filename)
{
  m_in.reset(new FilePiece(filename.c_str()));
  readNext();
}

ScoreDataIterator::~ScoreDataIterator() {}

void ScoreDataIterator::readNext()
{
  m_next.clear();
  try {
    StringPiece marker = m_in->ReadDelimited();
    if (marker != StringPiece(SCORES_TXT_BEGIN)) {
      throw FileFormatException(m_in->FileName(), marker.as_string());
    }
    size_t sentenceId = m_in->ReadULong();
    size_t count = m_in->ReadULong();
    size_t length = m_in->ReadULong();
    m_in->ReadLine(); //ignore rest of line
    for (size_t i = 0; i < count; ++i) {
      StringPiece line = m_in->ReadLine();
      m_next.push_back(ScoreDataItem());
      for (TokenIter<AnyCharacter, true> token(line,AnyCharacter(" \t")); token; ++token) {
        float value = ParseFloat(*token);
        m_next.back().push_back(value);
      }
      if (length != m_next.back().size()) {
        throw FileFormatException(m_in->FileName(), line.as_string());
      }
    }
    StringPiece line = m_in->ReadLine();
    if (line != StringPiece(SCORES_TXT_END)) {
      throw FileFormatException(m_in->FileName(), line.as_string());
    }
  } catch (EndOfFileException& e) {
    m_in.reset();
  }
}

void ScoreDataIterator::increment()
{
  readNext();
}


bool ScoreDataIterator::equal(const ScoreDataIterator& rhs) const
{
  if (!m_in && !rhs.m_in) {
    return true;
  } else if (!m_in) {
    return false;
  } else if (!rhs.m_in) {
    return false;
  } else {
    return m_in->FileName() == rhs.m_in->FileName() &&
           m_in->Offset() == rhs.m_in->Offset();
  }
}


const vector<ScoreDataItem>& ScoreDataIterator::dereference() const
{
  return m_next;
}

}

