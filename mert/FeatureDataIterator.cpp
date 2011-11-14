// $Id$
// vim:tabstop=2

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

#include "util/string_piece.hh"
#include "util/tokenize_piece.hh"

#include "FeatureArray.h"
#include "FeatureDataIterator.h"


using namespace std;
using namespace util;



FeatureDataIterator::FeatureDataIterator() {}

FeatureDataIterator::FeatureDataIterator(const string filename) {
  m_in.reset(new FilePiece(filename.c_str()));
  readNext();
}

void FeatureDataIterator::readNext() {
  try {
    StringPiece marker = m_in->ReadDelimited();
    if (marker != StringPiece(FEATURES_TXT_BEGIN)) {
      throw FileFormatException(m_in->FileName(), marker.as_string());
    }
    size_t sentenceId = m_in->ReadULong();
    size_t count = m_in->ReadULong();
    cerr << "Expecting " << count << endl;
    m_in->ReadLine(); //discard rest of line
    for (size_t i = 0; i < count; ++i) {
      StringPiece line = m_in->ReadLine();
      for (util::TokenIter<util::AnyCharacter, true> token(line, util::AnyCharacter(" \t")); token; ++token) {
        //TODO: Create FeatureDataItem
        char* err_ind;
        float value = static_cast<float>(strtod(token->data(), &err_ind));
        if (err_ind == token->data()) {
          throw FileFormatException(m_in->FileName(), line.as_string());
        }
        cerr << value << ",";
      }
      cerr << "\n";
    }
    StringPiece line = m_in->ReadLine();
    if (line != StringPiece(FEATURES_TXT_END)) {
      throw FileFormatException(m_in->FileName(), line.as_string());
    }
  } catch (EndOfFileException &e) {
    m_in.reset();
  }
}

void FeatureDataIterator::increment() {
  readNext();
}

bool FeatureDataIterator::equal(const FeatureDataIterator& rhs) const {
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

const vector<FeatureDataItem>& FeatureDataIterator::dereference() const {
  return m_next;
}
