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
#include <sstream>
#include <boost/functional/hash.hpp>

#include "util/file_piece.hh"
#include "util/tokenize_piece.hh"

#include "FeatureArray.h"
#include "FeatureDataIterator.h"


using namespace std;
using namespace util;

namespace MosesTuning
{


int ParseInt(const StringPiece& str )
{
  char* errIndex;
  //could wrap?
  int value = static_cast<int>(strtol(str.data(), &errIndex,10));
  if (errIndex == str.data()) {
    throw util::ParseNumberException(str);
  }
  return value;
}

float ParseFloat(const StringPiece& str)
{
  char* errIndex;
  float value = static_cast<float>(strtod(str.data(), &errIndex));
  if (errIndex == str.data()) {
    throw util::ParseNumberException(str);
  }
  return value;
}

bool operator==(FeatureDataItem const& item1, FeatureDataItem const& item2)
{
  return item1.dense==item1.dense && item1.sparse==item1.sparse;
}

size_t hash_value(FeatureDataItem const& item)
{
  size_t seed = 0;
  boost::hash_combine(seed,item.dense);
  boost::hash_combine(seed,item.sparse);
  return seed;
}


FeatureDataIterator::FeatureDataIterator() {}

FeatureDataIterator::FeatureDataIterator(const string& filename)
{
  m_in.reset(new FilePiece(filename.c_str()));
  readNext();
}

FeatureDataIterator::~FeatureDataIterator() {}

void FeatureDataIterator::readNext()
{
  m_next.clear();
  try {
    StringPiece marker = m_in->ReadDelimited();
    if (marker != StringPiece(FEATURES_TXT_BEGIN)) {
      throw FileFormatException(m_in->FileName(), marker.as_string());
    }
    size_t sentenceId = m_in->ReadULong();
    size_t count = m_in->ReadULong();
    size_t length = m_in->ReadULong();
    m_in->ReadLine(); //discard rest of line
    for (size_t i = 0; i < count; ++i) {
      StringPiece line = m_in->ReadLine();
      m_next.push_back(FeatureDataItem());
      for (TokenIter<AnyCharacter, true> token(line, AnyCharacter(" \t")); token; ++token) {
        TokenIter<AnyCharacterLast,false> value(*token,AnyCharacterLast("="));
        if (!value) throw FileFormatException(m_in->FileName(), line.as_string());
        StringPiece first = *value;
        ++value;
        if (!value) {
          //regular feature
          float floatValue = ParseFloat(first);
          m_next.back().dense.push_back(floatValue);
        } else {
          //sparse feature
          StringPiece second = *value;
          float floatValue = ParseFloat(second);
          m_next.back().sparse.set(first.as_string(),floatValue);
        }
      }
      if (length != m_next.back().dense.size()) {
        throw FileFormatException(m_in->FileName(), line.as_string());
      }
    }
    StringPiece line = m_in->ReadLine();
    if (line != StringPiece(FEATURES_TXT_END)) {
      throw FileFormatException(m_in->FileName(), line.as_string());
    }
  } catch (EndOfFileException &e) {
    m_in.reset();
  }
}

void FeatureDataIterator::increment()
{
  readNext();
}

bool FeatureDataIterator::equal(const FeatureDataIterator& rhs) const
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

const vector<FeatureDataItem>& FeatureDataIterator::dereference() const
{
  return m_next;
}

}

