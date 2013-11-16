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
#include <map>
#include "moses/TypeDef.h"


namespace OnDiskPt
{

class OnDiskWrapper;

/* A bidirectional map of string<->contiguous id
 * No distinction between source and target language
 */
class Vocab
{
protected:
  typedef std::map<std::string, UINT64> CollType;
  CollType m_vocabColl;

  std::vector<std::string> m_lookup; // opposite of m_vocabColl
  UINT64 m_nextId; // starts @ 1

public:
  Vocab()
    :m_nextId(1) {
  }
  UINT64 AddVocabId(const std::string &str);
  UINT64 GetVocabId(const std::string &str, bool &found) const;
  const std::string &GetString(UINT32 vocabId) const {
    return m_lookup[vocabId];
  }

  bool Load(OnDiskWrapper &onDiskWrapper);
  void Save(OnDiskWrapper &onDiskWrapper);
};

}

